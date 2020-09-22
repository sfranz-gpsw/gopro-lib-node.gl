#include "MeshTool.h"
#include "graphics/MeshUtil.h"
#include "DebugUtil.h"
using namespace ngfx;

int main(int argc, char** argv) {
	if (argc != 3) ERR("usage: ./MeshUtil <input> <output>");
	MeshData meshData;
	MeshTool::importPLY(argv[1], meshData);
	MeshUtil::exportMesh(argv[2], meshData);
}