//
//   Copyright 2013 Pixar
//
//   Licensed under the Apache License, Version 2.0 (the "Apache License")
//   with the following modification; you may not use this file except in
//   compliance with the Apache License and the following modification to it:
//   Section 6. Trademarks. is deleted and replaced with:
//
//   6. Trademarks. This License does not grant permission to use the trade
//      names, trademarks, service marks, or product names of the Licensor
//      and its affiliates, except as required to comply with Section 4(c) of
//      the License and to reproduce the content of the NOTICE file.
//
//   You may obtain a copy of the Apache License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the Apache License with the above modification is
//   distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
//   KIND, either express or implied. See the Apache License for the specific
//   language governing permissions and limitations under the Apache License.
//


//------------------------------------------------------------------------------
// Tutorial description:
//
// This tutorial presents in a very succint way the requisite steps to
// instantiate an Hbr mesh from simple topological data.
//

#include <stdio.h>

#include <far/refineTablesFactory.h>

//------------------------------------------------------------------------------
// Vertex container implementation.
//
struct Vertex {

    // Hbr minimal required interface ----------------------
    Vertex() { }

    Vertex(int /*i*/) { }

    Vertex(Vertex const & src) {
        _position[0] = src._position[0];
        _position[1] = src._position[1];
        _position[1] = src._position[1];
    }

    void Clear( void * =0 ) {
        _position[0]=_position[1]=_position[2]=0.0f;
    }

    void AddWithWeight(Vertex const & src, float weight) {
        _position[0]+=weight*src._position[0];
        _position[1]+=weight*src._position[1];
        _position[2]+=weight*src._position[2];
    }

    void AddVaryingWithWeight(Vertex const &, float) { }

    // Public interface ------------------------------------
    void SetPosition(float x, float y, float z) {
        _position[0]=x;
        _position[1]=y;
        _position[2]=z;
    }

    const float * GetPosition() const {
        return _position;
    }

private:
    float _position[3];
};

//------------------------------------------------------------------------------
// Pyramid geometry from catmark_pyramid.h
static float g_verts[8][3] = {{ -0.5f, -0.5f,  0.5f },
                              {  0.5f, -0.5f,  0.5f },
                              { -0.5f,  0.5f,  0.5f },
                              {  0.5f,  0.5f,  0.5f },
                              { -0.5f,  0.5f, -0.5f },
                              {  0.5f,  0.5f, -0.5f },
                              { -0.5f, -0.5f, -0.5f },
                              {  0.5f, -0.5f, -0.5f }};

static unsigned int g_nverts = 8,
                    g_nfaces = 6;

static unsigned int g_vertsperface[6] = { 4, 4, 4, 4, 4, 4 };

static unsigned int g_vertIndices[24] = { 0, 1, 3, 2,
                                          2, 3, 5, 4,
                                          4, 5, 7, 6,
                                          6, 7, 1, 0,
                                          1, 7, 5, 3,
                                          6, 0, 2, 4  };

using namespace OpenSubdiv;

//------------------------------------------------------------------------------
int main(int, char **) {

    SdcType type = TYPE_CATMARK;

    SdcOptions options;
    options.SetVVarBoundaryInterpolation(SdcOptions::VVAR_BOUNDARY_EDGE_ONLY);

    // Instantiate a factory
    FarRefineTablesFactoryBase factory;

    // Instantiate a FarRefineTables from the raw data
    FarRefineTables * refTables = factory.Create(type,
                                                 options,
                                                 g_nverts,
                                                 g_nfaces,
                                                 g_vertsperface,
                                                 g_vertIndices);

    int maxlevel = 2;

    // Uniformly refine the topolgy up to 'maxlevel'
    refTables->RefineUniform( maxlevel );


    // Allocate a buffer for vertex primvar data. The buffer length is set to
    // be the sum of all children vertices up to the highest level of refinement.
    std::vector<Vertex> vbuffer(refTables->GetNumVerticesTotal());
    Vertex * verts = &vbuffer[0];


    // Initialize coarse mesh positions
    int nCoarseVerts = g_nverts;
    for (int i=0; i<nCoarseVerts; ++i) {
        verts[i].SetPosition(g_verts[i][0], g_verts[i][1], g_verts[i][2]);
    }


    // Interpolate vertex primvar data
    refTables->Interpolate<Vertex>(verts, verts + nCoarseVerts);



    { // Output OBJ of the highest level refined -----------

        // Print vertex positions
        for (int level=0, firstVert=0; level<=maxlevel; ++level) {

            if (level==maxlevel) {
                for (int vert=0; vert<refTables->GetNumVertices(maxlevel); ++vert) {
                    float const * pos = verts[firstVert+vert].GetPosition();
                    printf("v %f %f %f\n", pos[0], pos[1], pos[2]);
                }
            } else {
                firstVert += refTables->GetNumVertices(level);
            }
        }

        // Print faces
        for (int face=0; face<refTables->GetNumFaces(maxlevel); ++face) {

            FarIndexArray fverts = refTables->GetFaceVertices(maxlevel, face);

            // all refined Catmark faces should be quads
            assert(fverts.size()==4);

            printf("f ");
            for (int vert=0; vert<fverts.size(); ++vert) {
                printf("%d ", fverts[vert]+1); // OBJ uses 1-based arrays...
            }
            printf("\n");
        }
    }
}

//------------------------------------------------------------------------------