#include "component/RenderComponent.hpp"
#include "server/Server.hpp"
#include "MyRender.hpp"
#include "Camera.hpp"
#include "scene/Scene.hpp"
using namespace std;
using namespace NRenderer;
namespace MyRender
{
	class Adapter : public RenderComponent
	{
		void render(SharedScene spScene) {
			MyRenderer photonrender{ spScene };
			auto result = photonrender.render();
			auto [pixels, width, height] = result;
			getServer().screen.set(pixels, width, height);
			photonrender.release(result);
		}
	};
}

// REGISTER_RENDERER(Name, Description, Class)
const static string description = "Only for test. (This is description)";
REGISTER_RENDERER(test, description, MyRender::Adapter);