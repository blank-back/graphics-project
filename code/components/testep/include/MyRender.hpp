#pragma once

#ifndef __MY_RENDERER_HPP__
#define __MY_RENDERER_HPP__

#include "scene/Scene.hpp"
#include "Ray.hpp"
#include "Camera.hpp"
#include "intersections/intersections.hpp"
#include "intersections/HitRecord.hpp"
#include "shaders/ShaderCreator.hpp"
#include <tuple>
#include <algorithm>
#include <vector>
#include "Onb.hpp"
namespace MyRender
{
	using namespace NRenderer;
	struct photon
	{
		MyRender::Vec3 position;
		MyRender::Vec3 r;
		MyRender::Ray in, out;
		MyRender::Vec3 norm;
		float operator[](int index) const {
			return position[index];
		}
		float& operator[](int index) {
			return position[index];
		}
	};
	class MyRenderer
	{
	public:

	private:
		SharedScene spScene;
		Scene& scene;
		MyRender::Camera camera;
		int width, height, depth, samplenum;
		int rendernum;

		vector<photon>photoncontainer;
		vector<SharedShader> shaderPrograms;
	public:
		MyRenderer(SharedScene spScene)
			: spScene(spScene)
			, scene(*spScene)
			, camera(spScene->camera)
			, rendernum(2000)//光子数，默认2000
		{
			//sceneui调整输入的渲染参数

			width = scene.renderOption.width;
			height = scene.renderOption.height;
			depth = scene.renderOption.depth;
			samplenum = scene.renderOption.samplesPerPixel;
		}
		~MyRenderer() = default;

		using RenderResult = tuple<RGBA*, unsigned int, unsigned int>;
		RenderResult render();
		void release(const RenderResult& r);

		//private:
		RGB gamma(const RGB& rgb);
		//pass one, send photons to make photon map
		void photontrace(Ray& ray, Vec3 r, int depth);
		void photonshoot();
		//bool getdistance(photon a, photon b);
		vector<photon> findneighbor(float& disrecord, int cap);
		//void raytrace();
		//pass two, ray trace
		RGB trace(const Ray& r, int currDepth);
		tuple<HitRecord, bool> closestHit(const Ray& r, bool isarea = true);
	};
}

#endif