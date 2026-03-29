#include "sm_mesh_topology.h"
#include <string.h>

// Small helper: sort (a,b) unordered pair into canonical order
static void SortPair(int *a, int *b)
{
    if (*a > *b) {
        int t = *a;
        *a = *b;
        *b = t;
    }
}

// ---- Open-addressing hash table for O(1) edge lookup ----
// Replaces the O(E) linear scan that made Topology_Build O(E^2).
// Uses Fibonacci hashing on the (a,b) vertex pair.
#define EDGE_HASH_SIZE 4096  // Power of 2, must be >= 2 * SM_MAX_EDGES
#define EDGE_HASH_MASK (EDGE_HASH_SIZE - 1)
#define EDGE_HASH_EMPTY (-1)

static int edge_hash_table[EDGE_HASH_SIZE]; // Maps hash slot -> edge index

static unsigned int edge_hash_key(int a, int b) {
    unsigned int h = (unsigned int)a * 2654435761u ^ (unsigned int)b * 2246822519u;
    return h & EDGE_HASH_MASK;
}

static int edge_hash_find(const MeshEdge *edges, int a, int b) {
    unsigned int slot = edge_hash_key(a, b);
    for (int probe = 0; probe < 64; probe++) {
        int idx = edge_hash_table[(slot + (unsigned)probe) & EDGE_HASH_MASK];
        if (idx == EDGE_HASH_EMPTY) return -1;
        if (edges[idx].v0 == a && edges[idx].v1 == b) return idx;
    }
    return -1; // Fallback: not found after 64 probes
}

static void edge_hash_insert(int edge_idx, int a, int b) {
    unsigned int slot = edge_hash_key(a, b);
    for (int probe = 0; probe < 64; probe++) {
        unsigned int s = (slot + (unsigned)probe) & EDGE_HASH_MASK;
        if (edge_hash_table[s] == EDGE_HASH_EMPTY) {
            edge_hash_table[s] = edge_idx;
            return;
        }
    }
    // Table full -- should not happen with proper sizing
}

void Topology_Build(MeshTopology *topo,
                    EditTri *tris, int triCount,
                    int vertCount)
{
    (void)vertCount;

    topo->edgeCount = 0;
    memset(edge_hash_table, 0xFF, sizeof(edge_hash_table)); // Fill with EDGE_HASH_EMPTY (-1)

    for (int ti = 0; ti < triCount; ++ti) {
        EditTri *t = &tris[ti];
        int v[3] = { t->v[0], t->v[1], t->v[2] };

        for (int e = 0; e < 3; ++e) {
            int a = v[e];
            int b = v[(e + 1) % 3];
            if (a < 0 || b < 0) continue;

            SortPair(&a, &b);

            int found = edge_hash_find(topo->edges, a, b);

            if (found >= 0) {
                MeshEdge *edge = &topo->edges[found];
                if (edge->tri0 == -1)      edge->tri0 = ti;
                else if (edge->tri1 == -1) edge->tri1 = ti;
            } else {
                if (topo->edgeCount >= SM_MAX_EDGES) continue;
                int idx = topo->edgeCount++;
                MeshEdge *edge = &topo->edges[idx];
                edge->v0   = a;
                edge->v1   = b;
                edge->tri0 = ti;
                edge->tri1 = -1;
                edge_hash_insert(idx, a, b);
            }
        }
    }
}

int Topology_FindEdge(const MeshTopology *topo,
                      int v0, int v1)
{
    SortPair(&v0, &v1);
    for (int i = 0; i < topo->edgeCount; ++i) {
        const MeshEdge *e = &topo->edges[i];
        if (e->v0 == v0 && e->v1 == v1) return i;
    }
    return -1;
}

// Core logic: Blender-style edge bevel (single segment)
// Takes the (v0,v1) edge, adds two new vertices at the endpoints,
// and splits tri0 and tri1 into 4 triangles.
int Mesh_BevelEdge(const MeshTopology *topo,
                   int edgeIndex,
                   int segments,
                   float amount,
                   EditVertex *verts,
                   int *pVertCount,
                   EditTri *tris,
                   int *pTriCount)
{
    if (!topo || !verts || !pVertCount || !tris || !pTriCount) return 0;
    if (edgeIndex < 0 || edgeIndex >= topo->edgeCount) return 0;

    MeshEdge e = topo->edges[edgeIndex];

    int vertCount = *pVertCount;
    int triCount  = *pTriCount;

    // Only bevel interior edges (shared by 2 faces) for now
    if (e.tri0 < 0 || e.tri1 < 0) {
        // Border edge bevel can be added later
        return 0;
    }

    if (vertCount + 2 > MAX_VERTS) return 0; // two new vertices
    if (triCount  + 2 > MAX_TRIS)  return 0; // two new triangles

    int v0 = e.v0;
    int v1 = e.v1;

    // Opposite vertex for tri0 (c)
    EditTri *t0 = &tris[e.tri0];
    int c = -1;
    for (int i = 0; i < 3; ++i) {
        int vi = t0->v[i];
        if (vi != v0 && vi != v1) {
            c = vi;
            break;
        }
    }

    // Opposite vertex for tri1 (d)
    EditTri *t1 = &tris[e.tri1];
    int d = -1;
    for (int i = 0; i < 3; ++i) {
        int vi = t1->v[i];
        if (vi != v0 && vi != v1) {
            d = vi;
            break;
        }
    }

    if (c < 0 || d < 0) {
        // Topology is broken, abort
        return 0;
    }

    Vector3 p0 = verts[v0].pos;
    Vector3 p1 = verts[v1].pos;
    Vector3 pc = verts[c].pos;
    Vector3 pd = verts[d].pos;

    // Compute face normals for both tris
    Vector3 n0 = Vector3Normalize(Vector3CrossProduct(
                      Vector3Subtract(p1, p0),
                      Vector3Subtract(pc, p0)));

    Vector3 n1 = Vector3Normalize(Vector3CrossProduct(
                      Vector3Subtract(p0, p1),
                      Vector3Subtract(pd, p1)));

    Vector3 nAvg = Vector3Add(n0, n1);
    if (Vector3Length(nAvg) < 1e-6f) {
        nAvg = n0; // fallback
    }
    nAvg = Vector3Normalize(nAvg);

    if (segments < 1) segments = 1; // only 1 segment for now
    float t = amount;

    // Bevel positions at edge endpoints
    Vector3 p0b = Vector3Add(p0, Vector3Scale(nAvg, t));
    Vector3 p1b = Vector3Add(p1, Vector3Scale(nAvg, t));

    // Add two new vertices
    int bv0 = vertCount++;
    int bv1 = vertCount++;

    verts[bv0].pos = p0b;
    verts[bv1].pos = p1b;

    // Rewrite the two existing triangles + add two new triangles.
    // This creates a chamfer band around the edge using 4 triangles
    // total:

    // tri0: (v0, c, bv0)  ve yeni tri: (bv0, c, bv1)
    // tri1: (v1, d, bv1)  ve yeni tri: (bv1, d, bv0)

    // Update tri0
    t0->v[0] = v0;
    t0->v[1] = c;
    t0->v[2] = bv0;

    // Update tri1
    t1->v[0] = v1;
    t1->v[1] = d;
    t1->v[2] = bv1;

    // yeni tri2
    EditTri *t2 = &tris[triCount++];
    t2->v[0] = bv0;
    t2->v[1] = c;
    t2->v[2] = bv1;

    // yeni tri3
    EditTri *t3 = &tris[triCount++];
    t3->v[0] = bv1;
    t3->v[1] = d;
    t3->v[2] = bv0;

    *pVertCount = vertCount;
    *pTriCount  = triCount;

    return 1;
}