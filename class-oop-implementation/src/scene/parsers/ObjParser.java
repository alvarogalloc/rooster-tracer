package scene.parsers;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import math.Vector2D;
import math.Vector3D;
import objects.Mesh;
import objects.Triangle;
import scene.Material;
import scene.Scene;
import scene.Texture;

public class ObjParser {

    @FunctionalInterface
    private interface ObjLineHandler {
        void handle(String[] tokens) throws IOException;
    }

    @FunctionalInterface
    private interface MtlLineHandler {
        void handle(String[] tokens) throws IOException;
    }

    private static final class MtlMaterial {
        String name;
        Vector3D ambient;
        Vector3D diffuse;
        Vector3D specular;
        Float shininess;
        String diffuseMap;
        String normalMap;
    }

    private static final class FloatList {
        float[] data = new float[1024];
        int size = 0;

        void add(float val) {
            if (size >= data.length) {
                float[] temp = new float[data.length * 2];
                System.arraycopy(data, 0, temp, 0, size);
                data = temp;
            }
            data[size++] = val;
        }

        void add(float val1, float val2) {
            add(val1);
            add(val2);
        }

        void add(float val1, float val2, float val3) {
            add(val1);
            add(val2);
            add(val3);
        }

        float get(int idx) {
            return data[idx];
        }
    }

    private static final class IntList {
        int[] data = new int[1024];
        int size = 0;

        void add(int val) {
            if (size >= data.length) {
                int[] temp = new int[data.length * 2];
                System.arraycopy(data, 0, temp, 0, size);
                data = temp;
            }
            data[size++] = val;
        }

        int get(int idx) {
            return data[idx];
        }
    }

    public static void parseObj(Scene scene, String[] tokens) throws IOException {
        if (tokens.length < 2) {
            System.err.println("Invalid obj format. Expected: obj <path/to/model.obj>");
            return;
        }

        String filename = resolveObjPath(scene, tokens[1]);
        Vector3D origin = new Vector3D(0, 0, 0);
        if (tokens.length >= 5) {
            try {
                origin = new Vector3D(
                    Float.parseFloat(tokens[2]),
                    Float.parseFloat(tokens[3]),
                    Float.parseFloat(tokens[4])
                );
            } catch (NumberFormatException e) {
                throw new IOException("Error parsing OBJ origin: " + e.getMessage(), e);
            }
        }

        Integer materialIdArg = null;
        if (tokens.length >= 6) {
            try {
                materialIdArg = Integer.parseInt(tokens[5]);
            } catch (NumberFormatException e) {
                throw new IOException("Error parsing OBJ material id: " + e.getMessage(), e);
            }
        }
        int materialId = resolveMaterialId(scene, materialIdArg);

        ObjLoader loader = new ObjLoader(scene, filename, origin, materialId);
        loader.load();
    }

    private static String resolveObjPath(Scene scene, String objPath) {
        File path = new File(objPath);
        if (path.isAbsolute()) {
            return path.getPath();
        }
        return new File(scene.getSourceDir(), objPath).getPath();
    }

    private static int resolveMaterialId(Scene scene, Integer materialIdArg) throws IOException {
        if (materialIdArg != null) {
            if (scene.getMaterials().isEmpty()) {
                throw new IOException("obj parser: material index provided but no materials are defined");
            }
            if (materialIdArg < 0 || materialIdArg >= scene.getMaterials().size()) {
                throw new IOException("obj parser: material index " + materialIdArg + " out of range [0, "
                    + scene.getMaterials().size() + ")");
            }
            return materialIdArg;
        }
        if (scene.getMaterials().isEmpty()) {
            throw new IOException("obj parser: material out of bounds");
        }
        return 0;
    }

    private static String resolveMtlPath(String mtlPath, String objPath) {
        File path = new File(mtlPath);
        if (path.isAbsolute()) {
            return path.getPath();
        }
        File base = new File(objPath).getAbsoluteFile().getParentFile();
        return new File(base, mtlPath).getPath();
    }

    private static String resolveAssetPath(String assetPath, String mtlPath) {
        File path = new File(assetPath);
        if (path.isAbsolute()) {
            return path.getPath();
        }
        File base = new File(mtlPath).getAbsoluteFile().getParentFile();
        return new File(base, assetPath).getPath();
    }

    private static final class ObjLoader {
        private static final float DEFAULT_AMBIENT_FACTOR = 0.1f;
        private static final float DEFAULT_SHININESS = 32f;

        private final Scene scene;
        private final String objFilename;
        private final Vector3D origin;
        private final int fallbackMaterialId;

        private final FloatList positions = new FloatList();
        private final FloatList uvs = new FloatList();
        private final FloatList normals = new FloatList();

        private final IntList faceVertIdx = new IntList();
        private final IntList faceUvIdx = new IntList();
        private final IntList faceNormalIdx = new IntList();
        private final IntList faceOffsets = new IntList();
        private final IntList faceMaterialIds = new IntList();

        private final Map<String, Integer> mtlIds = new HashMap<>();
        private final Map<String, Integer> textureIds = new HashMap<>();
        private int currentMaterialId;

        private final Map<String, ObjLineHandler> objHandlers = new HashMap<>();
        private final Map<String, MtlLineHandler> mtlHandlers = new HashMap<>();

        private MtlMaterial currentMtl = new MtlMaterial();
        private String currentMtlPath;

        public ObjLoader(Scene scene, String objFilename, Vector3D origin, int fallbackMaterialId) {
            this.scene = scene;
            this.objFilename = objFilename;
            this.origin = origin;
            this.fallbackMaterialId = fallbackMaterialId;
            this.currentMaterialId = fallbackMaterialId;

            initHandlers();
        }

        private void initHandlers() {
            objHandlers.put("v", this::handleVertex);
            objHandlers.put("vt", this::handleUv);
            objHandlers.put("vn", this::handleNormal);
            objHandlers.put("f", this::handleFace);
            objHandlers.put("mtllib", this::handleMtllib);
            objHandlers.put("usemtl", this::handleUsemtl);

            mtlHandlers.put("newmtl", this::handleNewmtl);
            mtlHandlers.put("Ka", this::handleKa);
            mtlHandlers.put("Kd", this::handleKd);
            mtlHandlers.put("Ks", this::handleKs);
            mtlHandlers.put("Ns", this::handleNs);
            mtlHandlers.put("map_Kd", this::handleMapKd);
            mtlHandlers.put("map_Bump", this::handleMapBump);
            mtlHandlers.put("map_bump", this::handleMapBump);
            mtlHandlers.put("bump", this::handleMapBump);
            mtlHandlers.put("map_Kn", this::handleMapBump);
            mtlHandlers.put("norm", this::handleMapBump);
        }

        public void load() throws IOException {
            System.out.println("loading obj: " + objFilename);
            File f = new File(objFilename);
            if (!f.isFile()) {
                throw new IOException("obj file (" + objFilename + ") not found");
            }

            try (BufferedReader reader = new BufferedReader(new FileReader(f))) {
                String line;
                while ((line = reader.readLine()) != null) {
                    line = line.trim();
                    if (line.isEmpty() || line.startsWith("#")) {
                        continue;
                    }
                    String[] tokens = line.split("\\s+");
                    if (tokens.length == 0) continue;
                    ObjLineHandler handler = objHandlers.get(tokens[0]);
                    if (handler != null) {
                        handler.handle(tokens);
                    }
                }
            }

            List<Triangle> triangles = new ArrayList<>();
            buildTriangles(triangles);

            if (!triangles.isEmpty()) {
                scene.add(new Mesh(triangles));
            }
            System.out.println("done loading model (vertex count: " + (positions.size / 3) + ", face count: " + triangles.size() + ")");
        }

        private void handleVertex(String[] tokens) {
            if (tokens.length < 4) return;
            float x = Float.parseFloat(tokens[1]) + (float) origin.getX();
            float y = Float.parseFloat(tokens[2]) + (float) origin.getY();
            float z = Float.parseFloat(tokens[3]) + (float) origin.getZ();
            positions.add(x, y, z);
        }

        private void handleUv(String[] tokens) {
            if (tokens.length < 3) return;
            float u = Float.parseFloat(tokens[1]);
            float v = Float.parseFloat(tokens[2]);
            uvs.add(u, v);
        }

        private void handleNormal(String[] tokens) {
            if (tokens.length < 4) return;
            float x = Float.parseFloat(tokens[1]);
            float y = Float.parseFloat(tokens[2]);
            float z = Float.parseFloat(tokens[3]);
            float len = (float) Math.sqrt(x * x + y * y + z * z);
            if (len > 1e-12f) {
                float inv = 1.0f / len;
                x *= inv;
                y *= inv;
                z *= inv;
            }
            normals.add(x, y, z);
        }

        private void handleMtllib(String[] tokens) throws IOException {
            for (int i = 1; i < tokens.length; i++) {
                String mtlPath = resolveMtlPath(tokens[i], objFilename);
                parseMtlFile(mtlPath);
            }
        }

        private void handleUsemtl(String[] tokens) {
            if (tokens.length >= 2) {
                currentMaterialId = mtlIds.getOrDefault(tokens[1], fallbackMaterialId);
            }
        }

        private void handleFace(String[] tokens) throws IOException {
            if (tokens.length < 4) return;
            faceOffsets.add(faceVertIdx.size);
            int totalVerts = positions.size / 3;
            int totalUvs = uvs.size / 2;
            int totalNormals = normals.size / 3;

            for (int i = 1; i < tokens.length; i++) {
                parseFaceToken(tokens[i], totalVerts, totalUvs, totalNormals);
            }
            faceMaterialIds.add(currentMaterialId);
        }

        private static int toZeroBasedIndex(int rawIndex, int count) {
            if (rawIndex > 0) {
                return rawIndex - 1;
            }
            if (rawIndex < 0) {
                return count + rawIndex;
            }
            return -1;
        }

        private void parseFaceToken(String token, int vertexCount, int uvCount, int normalCount) throws IOException {
            int firstSlash = token.indexOf('/');
            int secondSlash = firstSlash == -1 ? -1 : token.indexOf('/', firstSlash + 1);

            String vertexPart = firstSlash == -1 ? token : token.substring(0, firstSlash);
            if (vertexPart.isEmpty()) {
                throw new IOException("obj parser: malformed face vertex index");
            }

            int rawVertex = Integer.parseInt(vertexPart);
            int vertexIdx = toZeroBasedIndex(rawVertex, vertexCount);
            if (vertexIdx < 0 || vertexIdx >= vertexCount) {
                throw new IOException("obj parser: vertex index " + rawVertex + " out of range");
            }

            int uvIdx = -1;
            if (firstSlash != -1) {
                String uvPart = secondSlash == -1 ? token.substring(firstSlash + 1)
                    : token.substring(firstSlash + 1, secondSlash);
                if (!uvPart.isEmpty()) {
                    int rawUv = Integer.parseInt(uvPart);
                    uvIdx = toZeroBasedIndex(rawUv, uvCount);
                    if (uvIdx < 0 || uvIdx >= uvCount) {
                        throw new IOException("obj parser: uv index " + rawUv + " out of range");
                    }
                }
            }

            int normalIdx = -1;
            if (secondSlash != -1 && secondSlash + 1 < token.length()) {
                String normalPart = token.substring(secondSlash + 1);
                int rawNormal = Integer.parseInt(normalPart);
                normalIdx = toZeroBasedIndex(rawNormal, normalCount);
                if (normalIdx < 0 || normalIdx >= normalCount) {
                    throw new IOException("obj parser: normal index " + rawNormal + " out of range");
                }
            }

            faceVertIdx.add(vertexIdx);
            faceUvIdx.add(uvIdx);
            faceNormalIdx.add(normalIdx);
        }

        private void handleNewmtl(String[] tokens) throws IOException {
            registerMtlMaterial();
            currentMtl = new MtlMaterial();
            currentMtl.name = tokens.length > 1 ? tokens[1] : "";
        }

        private void handleKa(String[] tokens) {
            currentMtl.ambient = parseVec3Optional(tokens);
        }

        private void handleKd(String[] tokens) {
            currentMtl.diffuse = parseVec3Optional(tokens);
        }

        private void handleKs(String[] tokens) {
            currentMtl.specular = parseVec3Optional(tokens);
        }

        private void handleNs(String[] tokens) {
            currentMtl.shininess = parseFloatOptional(tokens);
        }

        private void handleMapKd(String[] tokens) {
            currentMtl.diffuseMap = tokens.length > 1 ? tokens[1] : null;
        }

        private void handleMapBump(String[] tokens) {
            String mapPath = tokens.length > 1 ? tokens[1] : null;
            if ("-bm".equals(mapPath) && tokens.length > 3) {
                mapPath = tokens[3];
            }
            currentMtl.normalMap = mapPath;
        }

        private void registerMtlMaterial() throws IOException {
            if (currentMtl.name == null || currentMtl.name.isEmpty()) {
                return;
            }
            if (mtlIds.containsKey(currentMtl.name)) {
                return;
            }

            Integer textureId = null;
            if (currentMtl.diffuseMap != null) {
                String path = resolveAssetPath(currentMtl.diffuseMap, currentMtlPath);
                textureId = loadTextureId(path);
            }

            Integer normalMapId = null;
            if (currentMtl.normalMap != null) {
                String path = resolveAssetPath(currentMtl.normalMap, currentMtlPath);
                normalMapId = loadTextureId(path);
            }

            Vector3D diffuse = currentMtl.diffuse != null ? currentMtl.diffuse : new Vector3D(1f, 1f, 1f);
            Vector3D ambient;
            if (currentMtl.ambient != null) {
                Vector3D ka = currentMtl.ambient;
                boolean isWhiteKa = ka.getX() >= 0.99f && ka.getY() >= 0.99f && ka.getZ() >= 0.99f;
                ambient = isWhiteKa ? diffuse.mul(DEFAULT_AMBIENT_FACTOR) : ka.mul(DEFAULT_AMBIENT_FACTOR);
            } else {
                ambient = diffuse.mul(DEFAULT_AMBIENT_FACTOR);
            }

            Vector3D specular = currentMtl.specular != null ? currentMtl.specular : new Vector3D(0.5f, 0.5f, 0.5f);
            float shininess = currentMtl.shininess != null ? currentMtl.shininess : DEFAULT_SHININESS;

            scene.addMaterial(new Material(ambient, diffuse, specular, shininess, textureId, normalMapId));
            int id = scene.getMaterials().size() - 1;
            mtlIds.put(currentMtl.name, id);
            System.out.println("parsed mtl material name=" + currentMtl.name + " id=" + id);
        }

        private int loadTextureId(String path) throws IOException {
            Integer existing = textureIds.get(path);
            if (existing != null) {
                return existing;
            }
            int id = scene.addTexture(Texture.load(path));
            textureIds.put(path, id);
            System.out.println("loaded texture id=" + id + " path=" + path);
            return id;
        }

        private void parseMtlFile(String filename) throws IOException {
            File f = new File(filename);
            if (!f.isFile()) {
                System.err.println("obj parser: mtl file not found (" + filename + ")");
                return;
            }
            this.currentMtlPath = filename;
            try (BufferedReader reader = new BufferedReader(new FileReader(f))) {
                String line;
                while ((line = reader.readLine()) != null) {
                    line = line.trim();
                    if (line.isEmpty() || line.startsWith("#")) {
                        continue;
                    }
                    String[] tokens = line.split("\\s+");
                    if (tokens.length == 0) continue;
                    MtlLineHandler handler = mtlHandlers.get(tokens[0]);
                    if (handler != null) {
                        handler.handle(tokens);
                    }
                }
            }
            registerMtlMaterial();
        }

        private Vector3D parseVec3Optional(String[] tokens) {
            if (tokens.length < 4) {
                return null;
            }
            try {
                return new Vector3D(Float.parseFloat(tokens[1]), Float.parseFloat(tokens[2]), Float.parseFloat(tokens[3]));
            } catch (NumberFormatException e) {
                return null;
            }
        }

        private Float parseFloatOptional(String[] tokens) {
            if (tokens.length < 2) {
                return null;
            }
            try {
                return Float.parseFloat(tokens[1]);
            } catch (NumberFormatException e) {
                return null;
            }
        }

        private Vector3D getPosition(int idx) {
            return new Vector3D(positions.get(3 * idx), positions.get(3 * idx + 1), positions.get(3 * idx + 2));
        }

        private Vector2D getUv(int idx) {
            if (idx < 0) {
                return new Vector2D(0f, 0f);
            }
            return new Vector2D(uvs.get(2 * idx), uvs.get(2 * idx + 1));
        }

        private Vector3D getNormal(int vertIdx, int normalIdx, float[] generatedNormals, Vector3D fallback) {
            if (normalIdx >= 0) {
                return new Vector3D(normals.get(3 * normalIdx), normals.get(3 * normalIdx + 1), normals.get(3 * normalIdx + 2));
            }
            if (generatedNormals != null) {
                float x = generatedNormals[3 * vertIdx];
                float y = generatedNormals[3 * vertIdx + 1];
                float z = generatedNormals[3 * vertIdx + 2];
                if (x * x + y * y + z * z > 1e-12f) {
                    return new Vector3D(x, y, z);
                }
            }
            return fallback;
        }

        public void buildTriangles(List<Triangle> triangles) {
            int totalVerts = positions.size / 3;
            float[] generatedNormals = null;

            boolean needsGeneratedNormals = false;
            for (int i = 0; i < faceNormalIdx.size; i++) {
                if (faceNormalIdx.get(i) < 0) {
                    needsGeneratedNormals = true;
                    break;
                }
            }

            if (needsGeneratedNormals) {
                generatedNormals = buildGeneratedNormals(totalVerts);
            }

            int numFaces = faceOffsets.size;
            for (int j = 0; j < numFaces; j++) {
                int start = faceOffsets.get(j);
                int end = (j + 1 < numFaces) ? faceOffsets.get(j + 1) : faceVertIdx.size;
                int numVerts = end - start;
                int materialId = faceMaterialIds.get(j);

                for (int i = 1; i + 1 < numVerts; i++) {
                    int idx0 = start;
                    int idx1 = start + i;
                    int idx2 = start + i + 1;

                    Vector3D p0 = getPosition(faceVertIdx.get(idx0));
                    Vector3D p1 = getPosition(faceVertIdx.get(idx1));
                    Vector3D p2 = getPosition(faceVertIdx.get(idx2));
                    Vector3D fallbackNormal = faceNormal(p0, p1, p2);

                    Vector3D n0 = getNormal(faceVertIdx.get(idx0), faceNormalIdx.get(idx0), generatedNormals, fallbackNormal);
                    Vector3D n1 = getNormal(faceVertIdx.get(idx1), faceNormalIdx.get(idx1), generatedNormals, fallbackNormal);
                    Vector3D n2 = getNormal(faceVertIdx.get(idx2), faceNormalIdx.get(idx2), generatedNormals, fallbackNormal);

                    Vector2D uv0 = getUv(faceUvIdx.get(idx0));
                    Vector2D uv1 = getUv(faceUvIdx.get(idx1));
                    Vector2D uv2 = getUv(faceUvIdx.get(idx2));

                    triangles.add(new Triangle(p0, p1, p2, n0, n1, n2, uv0, uv1, uv2, materialId));
                }
            }
        }

        private float[] buildGeneratedNormals(int totalVerts) {
            float[] generated = new float[totalVerts * 3];
            int numFaces = faceOffsets.size;
            for (int j = 0; j < numFaces; j++) {
                int start = faceOffsets.get(j);
                int end = (j + 1 < numFaces) ? faceOffsets.get(j + 1) : faceVertIdx.size;
                int numVerts = end - start;

                for (int i = 1; i + 1 < numVerts; i++) {
                    int idx0 = faceVertIdx.get(start);
                    int idx1 = faceVertIdx.get(start + i);
                    int idx2 = faceVertIdx.get(start + i + 1);

                    Vector3D p0 = getPosition(idx0);
                    Vector3D p1 = getPosition(idx1);
                    Vector3D p2 = getPosition(idx2);

                    Vector3D fn = faceNormal(p0, p1, p2);

                    addNormal(generated, idx0, fn);
                    addNormal(generated, idx1, fn);
                    addNormal(generated, idx2, fn);
                }
            }

            for (int i = 0; i < totalVerts; i++) {
                float x = generated[3 * i];
                float y = generated[3 * i + 1];
                float z = generated[3 * i + 2];
                float lenSq = x * x + y * y + z * z;
                if (lenSq > 1e-12f) {
                    float invLen = 1.0f / (float) Math.sqrt(lenSq);
                    generated[3 * i] = x * invLen;
                    generated[3 * i + 1] = y * invLen;
                    generated[3 * i + 2] = z * invLen;
                }
            }

            return generated;
        }

        private void addNormal(float[] generated, int vertIdx, Vector3D n) {
            generated[3 * vertIdx] += n.getX();
            generated[3 * vertIdx + 1] += n.getY();
            generated[3 * vertIdx + 2] += n.getZ();
        }

        private Vector3D faceNormal(Vector3D a, Vector3D b, Vector3D c) {
            return b.sub(a).cross(c.sub(a)).normalize();
        }
    }
}
