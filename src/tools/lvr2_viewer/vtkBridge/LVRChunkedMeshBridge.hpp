#ifndef LVR2_CHUNKED_MESH_BRIDGE_HPP_
#define LVR2_CHUNKED_MESH_BRIDGE_HPP_

#include <QObject>
//extern "C" {
//#include <GL/glx.h>
//}

#undef Success

#include <vtkSmartPointer.h>
#include <vtkActor.h>
#include <vtkActorCollection.h>
#include <vtkRenderer.h>
#include <vtkCamera.h>

#include "lvr2/algorithm/ChunkManager.hpp"
#include "lvr2/geometry/BoundingBox.hpp"
#include "lvr2/display/MeshOctree.hpp"

#include <string>

#include "MeshChunkActor.hpp"

#include <mutex>
#include <thread>
#include <condition_variable>



//class XContext;
//class XVisualInfo;
//class Window;
//struct XID;

//struct XVisualInfo;
//typedef struct XVisualInfo XVisualInfo;

//struct XVisualInfo;
////typedef struct XVisualInfo XVisualInfo;
//typedef unsigned long XID;
//typedef XID Window;
//struct _XDisplay;
//typedef struct _XDisplay Display;

typedef std::unordered_map<size_t, vtkSmartPointer<MeshChunkActor> > actorMap;
Q_DECLARE_METATYPE(actorMap)


//#include <GL/glx.h>
namespace lvr2 {
    class LVRChunkedMeshBridge : public QObject
    {
        Q_OBJECT
        public:
            LVRChunkedMeshBridge(std::string file, vtkSmartPointer<vtkRenderer> renderer, size_t cache_size = 1000, double highResDistance = 150.0);
            void getActors(double planes[24],
                    std::vector<BaseVector<float> >& centroids, 
                    std::vector<size_t >& indices);
        
            actorMap getHighResActors() { return m_highResActors; }
            actorMap getLowResActors() { return m_chunkActors; }
                    //std::unordered_map<size_t, vtkSmartPointer<MeshChunkActor> >& actors);
            void addInitialActors(vtkSmartPointer<vtkRenderer> renderer);

//            void fetchHighRes(double position[3], double dir[3], double up[3]);
            void fetchHighRes(BoundingBox<BaseVector<float > > bb, std::vector<size_t> indices);
        Q_SIGNALS:
            void updateHighRes(actorMap lowRes, actorMap highRes);
                    
                   // std::unordered_map<size_t, vtkSmartPointer<MeshChunkActor> > lowResActors,
                   //            std::unordered_map<size_t, vtkSmartPointer<MeshChunkActor> > highResActors);

        protected:
            void computeMeshActors();
            inline vtkSmartPointer<MeshChunkActor> computeMeshActor(size_t& id, MeshBufferPtr& chunk);

        private:
            vtkSmartPointer<vtkRenderer> m_renderer;
        //            Display* display;
    //        Window x_window;

            std::thread worker;
            std::mutex mutex;
            std::condition_variable cond_;
            double dist_;
            bool getNew_;
            bool running_;
            BoundingBox<BaseVector<float> > m_region;
            BoundingBox<BaseVector<float> > m_lastRegion;

            void highResWorker();
            lvr2::ChunkManager m_chunkManager;
            std::vector<size_t> highResIndices;
            std::unordered_map<size_t, MeshBufferPtr> m_chunks;
            std::unordered_map<size_t, MeshBufferPtr> m_highRes;
            std::unordered_map<size_t, vtkSmartPointer<MeshChunkActor> > m_chunkActors;
            std::unordered_map<size_t, vtkSmartPointer<MeshChunkActor> > m_highResActors;

            std::unique_ptr<MeshOctree<BaseVector<float> > > m_oct;
//            std::unordered_map<size_t, std::vector<vtkPolyData> > > m_chunkActors;
    };
} // namespace lvr2

#endif
