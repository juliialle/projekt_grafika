#ifndef OBJMODEL_H
#define OBJMODEL_H

#include "model.h"
#include <string>

namespace Models {

    class ObjModel : public Model {
    public:
        ObjModel(const std::string& filename);
        virtual ~ObjModel();
        virtual void drawSolid(bool smooth = true);

    private:
        void loadOBJ(const std::string& filename);
        void loadHardcodedCube();
        std::vector<glm::vec4> internalVertices;
        std::vector<glm::vec4> internalVertexNormals;
        std::vector<glm::vec4> internalFaceNormals;
        std::vector<glm::vec4> internalTexCoords;
    };

}

#endif