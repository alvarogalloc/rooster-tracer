package objects;

import java.util.ArrayList;
import java.util.List;
import java.util.Optional;
import math.AABB;
import math.Intersection;
import math.Interval;
import math.Ray;
import math.Vector3D;
import math.Vector2D;

public class Mesh implements Object3D {
    private static final int MAX_LEAF_TRIANGLES = 2;
    private static final int BIN_COUNT = 16;

    private final List<Triangle> triangles;
    private int[] triIndices;
    private List<BvhNode> nodes;
    private int rootIndex;

    private static class BvhNode {
        private final AABB bounds;
        private int leftChildOrFirstIndex;
        private int triangleCount;

        public BvhNode(AABB bounds, int leftChildOrFirstIndex, int triangleCount) {
            this.bounds = bounds;
            this.leftChildOrFirstIndex = leftChildOrFirstIndex;
            this.triangleCount = triangleCount;
        }

        public AABB getBounds() {
            return bounds;
        }

        public int getLeftChildOrFirstIndex() {
            return leftChildOrFirstIndex;
        }

        public void setLeftChildOrFirstIndex(int val) {
            this.leftChildOrFirstIndex = val;
        }

        public int getTriangleCount() {
            return triangleCount;
        }

        public void setTriangleCount(int val) {
            this.triangleCount = val;
        }

        public boolean isLeaf() {
            return triangleCount > 0;
        }
    }

    private static class Bin {
        AABB bounds = AABB.empty();
        int count = 0;
    }

    private static class BinSums {
        AABB[] bounds = new AABB[BIN_COUNT];
        int[] counts = new int[BIN_COUNT];
    }

    private static class SahSplit {
        int axis;
        int splitBin;
        int leftCount;
        int rightCount;
        AABB leftBounds;
        AABB rightBounds;
        float cost;
    }

    public Mesh(List<Triangle> triangles) {
        this.triangles = triangles;
        buildBvh();
    }

    public List<Triangle> getTriangles() {
        return triangles;
    }

    private static float axisValue(Vector3D v, int axis) {
        return axis == 0 ? v.getX() : axis == 1 ? v.getY() : v.getZ();
    }

    private static int binIndex(float c, float axisMin, float invBinSize) {
        int idx = (int) ((c - axisMin) * invBinSize);
        return Math.max(0, Math.min(idx, BIN_COUNT - 1));
    }

    private static AABB triangleBounds(Vector3D p0, Vector3D p1, Vector3D p2) {
        float minX = Math.min(p0.getX(), Math.min(p1.getX(), p2.getX()));
        float minY = Math.min(p0.getY(), Math.min(p1.getY(), p2.getY()));
        float minZ = Math.min(p0.getZ(), Math.min(p1.getZ(), p2.getZ()));

        float maxX = Math.max(p0.getX(), Math.max(p1.getX(), p2.getX()));
        float maxY = Math.max(p0.getY(), Math.max(p1.getY(), p2.getY()));
        float maxZ = Math.max(p0.getZ(), Math.max(p1.getZ(), p2.getZ()));

        return new AABB(new Vector3D(minX, minY, minZ), new Vector3D(maxX, maxY, maxZ));
    }

    private static AABB centroidBoundsFor(int[] triIndices, int first, int count, Vector3D[] centroids) {
        AABB bounds = AABB.empty();
        for (int i = 0; i < count; i++) {
            bounds = bounds.extend(centroids[triIndices[first + i]]);
        }
        return bounds;
    }

    private static boolean isDegenerate(AABB bounds) {
        if (bounds.isEmpty()) {
            return true;
        }
        Vector3D extent = bounds.getMax().sub(bounds.getMin());
        return extent.getX() <= 0f && extent.getY() <= 0f && extent.getZ() <= 0f;
    }

    private static Bin[] buildBins(
        int[] triIndices, int first, int count,
        Vector3D[] centroids, AABB[] triBounds,
        int axis, float axisMin, float invBinSize
    ) {
        Bin[] bins = new Bin[BIN_COUNT];
        for (int i = 0; i < BIN_COUNT; i++) {
            bins[i] = new Bin();
        }
        for (int idx = 0; idx < count; idx++) {
            int triIndex = triIndices[first + idx];
            float c = axisValue(centroids[triIndex], axis);
            int binIdx = binIndex(c, axisMin, invBinSize);
            bins[binIdx].count += 1;
            bins[binIdx].bounds = bins[binIdx].bounds.extend(triBounds[triIndex]);
        }
        return bins;
    }

    private static BinSums prefixSums(Bin[] bins) {
        BinSums sums = new BinSums();
        AABB runningBounds = AABB.empty();
        int runningCount = 0;
        for (int i = 0; i < BIN_COUNT; ++i) {
            runningCount += bins[i].count;
            sums.counts[i] = runningCount;
            runningBounds = runningBounds.extend(bins[i].bounds);
            sums.bounds[i] = runningBounds;
        }
        return sums;
    }

    private static BinSums suffixSums(Bin[] bins) {
        BinSums sums = new BinSums();
        AABB runningBounds = AABB.empty();
        int runningCount = 0;
        for (int i = BIN_COUNT - 1; i >= 0; --i) {
            runningCount += bins[i].count;
            sums.counts[i] = runningCount;
            runningBounds = runningBounds.extend(bins[i].bounds);
            sums.bounds[i] = runningBounds;
        }
        return sums;
    }

    private static SahSplit bestAxisSplit(
        int[] triIndices, int first, int count,
        Vector3D[] centroids, AABB[] triBounds,
        AABB centroidBounds, int axis
    ) {
        Vector3D extent = centroidBounds.getMax().sub(centroidBounds.getMin());
        float axisExtent = axisValue(extent, axis);
        if (axisExtent <= 1e-6f) {
            return null;
        }

        float axisMin = axisValue(centroidBounds.getMin(), axis);
        float invBinSize = (float) BIN_COUNT / axisExtent;
        Bin[] bins = buildBins(triIndices, first, count, centroids, triBounds, axis, axisMin, invBinSize);
        BinSums left = prefixSums(bins);
        BinSums right = suffixSums(bins);

        SahSplit best = null;
        float bestCost = Float.POSITIVE_INFINITY;
        for (int split = 0; split < BIN_COUNT - 1; ++split) {
            if (left.counts[split] == 0 || right.counts[split + 1] == 0) {
                continue;
            }

            float cost = left.counts[split] * left.bounds[split].surfaceArea() +
                         right.counts[split + 1] * right.bounds[split + 1].surfaceArea();
            if (cost < bestCost) {
                bestCost = cost;
                best = new SahSplit();
                best.axis = axis;
                best.splitBin = split;
                best.leftCount = left.counts[split];
                best.rightCount = right.counts[split + 1];
                best.leftBounds = left.bounds[split];
                best.rightBounds = right.bounds[split + 1];
                best.cost = cost;
            }
        }
        return best;
    }

    private static SahSplit findBestSplit(
        int[] triIndices, int first, int count,
        Vector3D[] centroids, AABB[] triBounds,
        AABB centroidBounds
    ) {
        SahSplit best = null;
        for (int axis = 0; axis < 3; ++axis) {
            SahSplit candidate = bestAxisSplit(triIndices, first, count, centroids, triBounds, centroidBounds, axis);
            if (candidate == null) {
                continue;
            }
            if (best == null || candidate.cost < best.cost) {
                best = candidate;
            }
        }
        return best;
    }

    private static int partitionBySplit(
        int[] triIndices, int first, int count,
        Vector3D[] centroids, AABB centroidBounds,
        SahSplit split
    ) {
        Vector3D extent = centroidBounds.getMax().sub(centroidBounds.getMin());
        float axisExtent = axisValue(extent, split.axis);
        if (axisExtent <= 1e-6f) {
            return 0;
        }

        float axisMin = axisValue(centroidBounds.getMin(), split.axis);
        float invBinSize = (float) BIN_COUNT / axisExtent;

        int left = first;
        int right = first + count - 1;
        while (left <= right) {
            int i = triIndices[left];
            float c = axisValue(centroids[i], split.axis);
            int binIdx = binIndex(c, axisMin, invBinSize);
            if (binIdx <= split.splitBin) {
                left++;
            } else {
                int temp = triIndices[left];
                triIndices[left] = triIndices[right];
                triIndices[right] = temp;
                right--;
            }
        }
        return left - first;
    }

    private void buildBvh() {
        int numTris = triangles.size();
        if (numTris == 0) {
            return;
        }

        Vector3D[] centroids = new Vector3D[numTris];
        AABB[] triBounds = new AABB[numTris];

        AABB meshBounds = AABB.empty();
        for (int i = 0; i < numTris; i++) {
            Triangle tri = triangles.get(i);
            centroids[i] = tri.getP0().add(tri.getP1()).add(tri.getP2()).mul(1f / 3f);
            triBounds[i] = triangleBounds(tri.getP0(), tri.getP1(), tri.getP2());
            meshBounds = meshBounds.extend(triBounds[i]);
        }

        this.triIndices = new int[numTris];
        for (int i = 0; i < numTris; i++) {
            triIndices[i] = i;
        }

        this.nodes = new ArrayList<>();
        this.nodes.add(new BvhNode(meshBounds, 0, numTris));
        this.rootIndex = 0;

        subdivide(this.nodes, this.triIndices, centroids, triBounds, 0);
    }

    private void subdivide(
        List<BvhNode> nodes, int[] triIndices,
        Vector3D[] centroids, AABB[] triBounds,
        int nodeIndex
    ) {
        BvhNode node = nodes.get(nodeIndex);
        if (node.getTriangleCount() <= MAX_LEAF_TRIANGLES) {
            return;
        }

        int first = node.getLeftChildOrFirstIndex();
        int count = node.getTriangleCount();

        AABB centroidBounds = centroidBoundsFor(triIndices, first, count, centroids);
        if (isDegenerate(centroidBounds)) {
            return;
        }

        SahSplit split = findBestSplit(triIndices, first, count, centroids, triBounds, centroidBounds);
        if (split == null) {
            return;
        }

        int leftCount = partitionBySplit(triIndices, first, count, centroids, centroidBounds, split);
        if (leftCount == 0 || leftCount == count) {
            return;
        }

        int leftIndex = nodes.size();
        nodes.add(new BvhNode(split.leftBounds, first, leftCount));
        nodes.add(new BvhNode(split.rightBounds, first + leftCount, count - leftCount));

        node.setLeftChildOrFirstIndex(leftIndex);
        node.setTriangleCount(0);

        subdivide(nodes, triIndices, centroids, triBounds, leftIndex);
        subdivide(nodes, triIndices, centroids, triBounds, leftIndex + 1);
    }

    @Override
    public Optional<Intersection> isHit(Ray ray, Interval tRange) {
        if (nodes == null || nodes.isEmpty()) {
            return Optional.empty();
        }

        Optional<Intersection> closestHit = Optional.empty();
        float validMin = tRange.getMin();
        float validMax = tRange.getMax();

        int[] stack = new int[64];
        int stackTop = 0;
        int current = rootIndex;

        while (true) {
            BvhNode node = nodes.get(current);

            if (node.isLeaf()) {
                for (int i = 0; i < node.getTriangleCount(); ++i) {
                    int triIdx = triIndices[node.getLeftChildOrFirstIndex() + i];
                    Triangle tri = triangles.get(triIdx);
                    
                    Optional<Intersection> hit = tri.isHit(ray, new Interval(validMin, validMax));
                    if (hit.isPresent()) {
                        validMax = hit.get().getT();
                        closestHit = hit;
                    }
                }

                if (stackTop == 0) {
                    break;
                }
                current = stack[--stackTop];
            } else {
                int left = node.getLeftChildOrFirstIndex();
                int right = left + 1;

                BvhNode leftNode = nodes.get(left);
                BvhNode rightNode = nodes.get(right);

                boolean hitLeft = leftNode.getBounds().isHit(ray, new Interval(validMin, validMax));
                boolean hitRight = rightNode.getBounds().isHit(ray, new Interval(validMin, validMax));

                if (!hitLeft && !hitRight) {
                    if (stackTop == 0) {
                        break;
                    }
                    current = stack[--stackTop];
                } else if (hitLeft && hitRight) {
                    stack[stackTop++] = right;
                    current = left;
                } else {
                    current = hitLeft ? left : right;
                }
            }
        }

        return closestHit;
    }
}
