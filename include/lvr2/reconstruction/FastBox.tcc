/* Copyright (C) 2011 Uni Osnabrück
 * This file is part of the LAS VEGAS Reconstruction Toolkit,
 *
 * LAS VEGAS is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * LAS VEGAS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
 */


 /*
 * FastBox.cpp
 *
 *  Created on: 03.03.2011
 *      Author: Thomas Wiemann
 */


namespace lvr2
{

template<typename BoxT>
const string BoxTraits<BoxT>::type = "FastBox";

template<typename BaseVecT>
float FastBox<BaseVecT>::m_voxelsize = 0;

template<typename BaseVecT>
uint FastBox<BaseVecT>::INVALID_INDEX = numeric_limits<uint>::max();

template<typename BaseVecT>
FastBox<BaseVecT>::FastBox(Point<BaseVecT> center)
{
    for(int i = 0; i < 8; i++)
    {
        m_vertices[i] = INVALID_INDEX;
    }

    for(int i = 0; i < 27; i++)
    {
        m_neighbors[i] = 0;
    }
    m_center = center;
}

template<typename BaseVecT>
void FastBox<BaseVecT>::setVertex(int index, uint nb)
{
    m_vertices[index] = nb;
}

template<typename BaseVecT>
void FastBox<BaseVecT>::setNeighbor(int index, FastBox<BaseVecT>* nb)
{
    m_neighbors[index] = nb;
}


template<typename BaseVecT>
FastBox<BaseVecT>* FastBox<BaseVecT>::getNeighbor(int index)
{
    return m_neighbors[index];
}

template<typename BaseVecT>
uint FastBox<BaseVecT>::getVertex(int index)
{
    return m_vertices[index];
}



template<typename BaseVecT>
void FastBox<BaseVecT>::getCorners(Point<BaseVecT> corners[],
                                           vector<QueryPoint<BaseVecT> > &qp)
{
    // Get the box corner positions from the query point array
    for(int i = 0; i < 8; i++)
    {
        corners[i] = qp[m_vertices[i]].m_position;
    }
}

template<typename BaseVecT>
void FastBox<BaseVecT>::getDistances(float distances[],
                                             vector<QueryPoint<BaseVecT> > &qp)
{
    // Get the distance values from the query point array
    // for the corners of the current box
    for(int i = 0; i < 8; i++)
    {
        distances[i] = qp[m_vertices[i]].m_distance;
    }
}

template<typename BaseVecT>
int  FastBox<BaseVecT>::getIndex(vector<QueryPoint<BaseVecT> > &qp)
{
    // Determine the MC-Table index for the current corner configuration
    int index = 0;
    for(int i = 0; i < 8; i++)
    {
        if(qp[m_vertices[i]].m_distance > 0) index |= (1 << i);
    }
    return index;
}

template<typename BaseVecT>
float FastBox<BaseVecT>::calcIntersection(float x1, float x2, float d1, float d2)
{

    // Calculate the surface intersection using linear interpolation
    // and check for different signs of the given distance values.
    // If for some reason there was no sign change, return the
    // middle point
    if( (d1 < 0 && d2 >= 0) || (d2 < 0 && d1 >= 0) )
    {
      float interpolation = x2 - d2 * (x1 - x2) / (d1 - d2);
      if(compareFloat(interpolation, x1))
        interpolation += 0.01;
      else if(compareFloat(interpolation, x2))
        interpolation -= 0.01;
      return  interpolation;
    }
    else
    {
      return  (x2 + x1) / 2.0;
    }
}

template<typename BaseVecT>
void FastBox<BaseVecT>::getIntersections(BaseVecT* corners,
                                                 float* distance,
                                                 Point<BaseVecT>* positions)
{
    float intersection;

    intersection = calcIntersection( (corners[0]).x, (corners[1]).x, distance[0], distance[1]);
    positions[0] = Point<BaseVecT>(intersection, corners[0].y, corners[0].z);

    intersection = calcIntersection(corners[1].y, corners[2].y, distance[1], distance[2]);
    positions[1] = Point<BaseVecT>(corners[1].x, intersection, corners[1].z);

    intersection = calcIntersection(corners[3].x, corners[2].x, distance[3], distance[2]);
    positions[2] = Point<BaseVecT>(intersection, corners[2].y, corners[2].z);

    intersection = calcIntersection(corners[0].y, corners[3].y, distance[0], distance[3]);
    positions[3] = Point<BaseVecT>(corners[3].x, intersection, corners[3].z);

    //Back Quad
    intersection = calcIntersection(corners[4].x, corners[5].x, distance[4], distance[5]);
    positions[4] = Point<BaseVecT>(intersection, corners[4].y, corners[4].z);

    intersection = calcIntersection(corners[5].y, corners[6].y, distance[5], distance[6]);
    positions[5] = Point<BaseVecT>(corners[5].x, intersection, corners[5].z);


    intersection = calcIntersection(corners[7].x, corners[6].x, distance[7], distance[6]);
    positions[6] = Point<BaseVecT>(intersection, corners[6].y, corners[6].z);

    intersection = calcIntersection(corners[4].y, corners[7].y, distance[4], distance[7]);
    positions[7] = Point<BaseVecT>(corners[7].x, intersection, corners[7].z);

    //Sides
    intersection = calcIntersection(corners[0].z, corners[4].z, distance[0], distance[4]);
    positions[8] = Point<BaseVecT>(corners[0].x, corners[0].y, intersection);

    intersection = calcIntersection(corners[1].z, corners[5].z, distance[1], distance[5]);
    positions[9] = Point<BaseVecT>(corners[1].x, corners[1].y, intersection);

    intersection = calcIntersection(corners[3].z, corners[7].z, distance[3], distance[7]);
    positions[10] = Point<BaseVecT>(corners[3].x, corners[3].y, intersection);

    intersection = calcIntersection(corners[2].z, corners[6].z, distance[2], distance[6]);
    positions[11] = Point<BaseVecT>(corners[2].x, corners[2].y, intersection);

}


template<typename BaseVecT>
void FastBox<BaseVecT>::getSurface(
    BaseMesh<BaseVecT>& mesh,
    vector<QueryPoint<BaseVecT>>& qp,
    uint &globalIndex
)
{
    Point<BaseVecT> corners[8];
    Point<BaseVecT> vertex_positions[12];

    float distances[8];

    getCorners(corners, qp);
    getDistances(distances, qp);
    getIntersections(corners, distances, vertex_positions);

    int index = getIndex(qp);

    // Do not create traingles for invalid boxes
    for (int i = 0; i < 8; i++)
    {
        if (qp[m_vertices[i]].m_invalid)
        {
            return;
        }
    }

    // Generate the local approximation surface according to the marching
    // cubes table for Paul Burke.
    for(int a = 0; lvr::MCTable[index][a] != -1; a+= 3)
    {
        OptionalVertexHandle vertex_indices[3];

        for(int b = 0; b < 3; b++)
        {
            auto edge_index = lvr::MCTable[index][a + b];

            //If no index was found generate new index and vertex
            //and update all neighbor boxes
            if(!m_intersections[edge_index])
            {
                auto v = vertex_positions[edge_index];
                m_intersections[edge_index] = mesh.addVertex(v);

                for(int i = 0; i < 3; i++)
                {
                    auto current_neighbor = m_neighbors[lvr::neighbor_table[edge_index][i]];
                    if(current_neighbor != 0)
                    {
                        current_neighbor->m_intersections[lvr::neighbor_vertex_table[edge_index][i]] = m_intersections[edge_index];
                    }
                }

                // Increase the global vertex counter to save the buffer
                // position were the next new vertex has to be inserted
                globalIndex++;
            }

            //Save vertex index in mesh
            vertex_indices[b] = m_intersections[edge_index];
        }

        // Add triangle actually does the normal interpolation for us.
        mesh.addFace(
            vertex_indices[0].unwrap(),
            vertex_indices[1].unwrap(),
            vertex_indices[2].unwrap()
        );
    }
}

} // namespace lvr2