#pragma once

#include "component.h"
#include "mesh.h"

namespace osp {

class MeshComponent : public Component {
private:
	Mesh* mesh;
	// Material* material;
};

}