import java.awt.Color;
import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;

public class SceneParser {
    public static Scene parseScene(String filePath) throws IOException {
        Scene scene = new Scene();

        try (BufferedReader reader = new BufferedReader(new FileReader(filePath))) {
            String header = reader.readLine();
            final String fileToken = "ROOSTERSCENEV1";
            if (header == null || !header.trim().equals(fileToken)) {
                throw new IOException("Invalid scene file format! Missing ROOSTERSCENEV1 header.");
            }

            String line;
            while ((line = reader.readLine()) != null) {
                line = line.trim();
                if (line.isEmpty() || line.startsWith("#")) { // Skip empty lines and comments
                    continue;
                }
                parseLine(scene, line);
            }
        }

        return scene;
    }

    private static void parseLine(Scene scene, String line) throws IOException {
        String[] tokens = line.split("\\s+");
        String type = tokens[0].toLowerCase();

        switch (type) {
            case "sphere":
                scene.add(parseSphere(tokens));
                break;
            default:
                System.out.println("Warning - Unknown object type skipped: " + type);
                break;
        }
    }
    
    // Described sphere attributes: sphere <x> <y> <z> <radius> <r> <g> <b> (r,b,g are 0-255)
    // example: "sphere 0 0 -5 1 255 0 0" for a red sphere at (0,0,-5) with radius 1
    private static Sphere parseSphere(String[] tokens) throws IOException {
        if (tokens.length != 8) {
            throw new IOException("Invalid sphere format. Expected: sphere <x> <y> <z> <radius> <r> <g> <b>");
        }

        try {
            float x = Float.parseFloat(tokens[1]);
            float y = Float.parseFloat(tokens[2]);
            float z = Float.parseFloat(tokens[3]);
            float radius = Float.parseFloat(tokens[4]);
            
            // Expected color components: integers from 0 to 255
            int r = Integer.parseInt(tokens[5]);
            int g = Integer.parseInt(tokens[6]);
            int b = Integer.parseInt(tokens[7]);

            return new Sphere(new Vector3D(x, y, z), radius, new Color(r, g, b));
        } catch (NumberFormatException e) {
            throw new IOException("Number parsing error for sphere values: " + e.getMessage());
        }
    }
}
