#include "MyRender.hpp"
#include "server/Server.hpp"
#include "VertexTransformer.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "shaders/Glass.hpp"
#include "intersections/intersections.hpp"
#include <assert.h>
namespace MyRender
{
	//hit point
	Vec3 nowcal = Vec3{ 0 };
	void MyRenderer::release(const RenderResult& r) {
		auto [p, w, h] = r;
		delete[] p;
	}
	RGB MyRenderer::gamma(const RGB& rgb) {
		return glm::sqrt(rgb);
	}
	Vec3 changeway(const Vec3& normal) {
		Vec3 random = defaultSamplerInstance<HemiSphere>().sample3d();
		Onb onb{ normal };
		Vec3 direction = glm::normalize(onb.local(random));
		return direction;
	}
	void MyRenderer::photontrace(Ray& ray, Vec3 r, int currDepth)
	{
		auto [hitone, islight] = closestHit(ray, false);
		if (!hitone)
			return;
		else
		{
			auto mtlHandle = hitone->material;
			auto scattered = shaderPrograms[mtlHandle.index()]->shade(ray, hitone->hitPoint, hitone->normal);
			auto attenu = scattered.attenuation;
			auto emitted = scattered.emitted;
			Ray next = scattered.ray;
			//漫反射
			if (scene.materials[mtlHandle.index()].hasProperty("diffuseColor"))
			{
				photon p;
				p.position = hitone->hitPoint;
				p.r = r;
				p.in = ray;
				p.out = next;
				p.norm = hitone->normal;
				photoncontainer.push_back(p);
				if (currDepth >= depth || rand() % 1000 < 200) return;
				float cos_0 = abs(glm::dot(hitone->normal, ray.direction));
				photontrace(next, attenu * r * cos_0 / (scattered.pdf * 0.8f), currDepth + 1);
			}
			//glass
			else if (scene.materials[mtlHandle.index()].hasProperty("ior"))
			{
				if (currDepth >= depth || rand() % 1000 < 200) return;
				photontrace(next, attenu * r / (scattered.pdf * 0.8f), currDepth + 1);
				next = scattered.refraction_ray;
				photontrace(next, scattered.refraction * r / (scattered.pdf * 0.8f), currDepth + 1);
			}
		}
	}
	//stage 1 photon trace
	void  MyRenderer::photonshoot() {
		//assert(scene.areaLightBuffer.size());
		//begin to send photons
		for (int i = 0; i < rendernum; i++) {
			for (int j = 0; j < scene.areaLightBuffer.size(); j++) {
				auto p1 = (rand() % 1000) / 1000.0f;
				auto p2 = (rand() % 1000) / 1000.0f;
				auto u1 = glm::normalize(scene.areaLightBuffer[j].u);
				auto v1 = glm::normalize(scene.areaLightBuffer[j].v);
				Vec3 random = changeway(glm::cross(u1, v1));
				auto origin = scene.areaLightBuffer[j].position + p1 * scene.areaLightBuffer[j].u + p2 * scene.areaLightBuffer[j].v;
				Ray ray{ origin, random };
				Vec3 r = (scene.areaLightBuffer[j].radiance * glm::length(glm::cross(scene.areaLightBuffer[j].u, scene.areaLightBuffer[j].v)) / PI);
				photontrace(ray, r, 0);
			}
		}
	}
	//calculate distance for sort
	bool getdistance(photon a, photon b)
	{
		return glm::length(a.position - nowcal) < glm::length(b.position - nowcal);
	}
	//stage 2 get neighbor photons
	vector<photon> MyRenderer::findneighbor(float& disrecord, int cap)
	{
		//should be replaced by other algorithms for sufficiency
		sort(photoncontainer.begin(), photoncontainer.end(), getdistance);
		std::vector<photon>bp;
		for (int i = 0; i < cap; i++)
			bp.push_back(photoncontainer[i]);
		disrecord = std::max(glm::length(bp[cap - 1].position - nowcal), disrecord);
		//in case that photons are on the same point
		disrecord = clamp(disrecord, 1000, 0.01);
		return bp;
	}
	//stage 2 ray trace
	RGB MyRenderer::trace(const Ray& r, int currDepth) {
		//if (currDepth == depth) return scene.ambient.constant;

		auto [HitOne, islight] = closestHit(r);
		if (HitOne) {
			if (islight)
			{
				return HitOne->normal;
			}
			else
			{
				auto mtlHandle = HitOne->material;
				auto scattered = shaderPrograms[mtlHandle.index()]->shade(r, HitOne->hitPoint, HitOne->normal);
				auto scatteredRay = scattered.ray;
				auto attenuation = scattered.attenuation;
				auto emitted = scattered.emitted;
				float dotpara = glm::dot(HitOne->normal, scatteredRay.direction);
				float pdf = scattered.pdf;
				float  disrecord = 0.f;
				Vec3 dir{ 0,0,0 };
				Vec3 sumreco{ 0,0,0 };
				if (scene.materials[mtlHandle.index()].hasProperty("ior")) {
					RGB next = Vec3(0.0f);
					//to control the optical path iength
					if (currDepth > depth && rand() % 1000 < 200) next = (attenuation + scattered.refraction) * scene.ambient.constant;
					else {
						auto reflex = trace(scattered.ray, currDepth + 1);
						RGB refraction = Vec3(0.f);
						if (scattered.refraction_ray.direction != Vec3(0.f))
							refraction = trace(scattered.refraction_ray, currDepth + 1);
						next = attenuation * reflex + refraction * scattered.refraction;
					}
					return emitted + next / (scattered.pdf * 0.8f);
				}
				nowcal = HitOne->hitPoint;
				auto tempvec = findneighbor(disrecord, 20);
				for (int i = 0; i < tempvec.size(); i++)
				{
					auto p = photoncontainer[i];
					float temp2 = glm::dot(-(p.in.direction), HitOne->normal);
					if (temp2 <= 0.f) continue;
					dir += -(p.in.direction);
					sumreco += p.r / (float)(PI * disrecord * disrecord * rendernum);
				}
				float dotans = glm::dot(HitOne->normal, glm::normalize(dir));
				return emitted + attenuation * sumreco * dotans / pdf;
			}
		}
		else {
			return Vec3{ 0 };
		}
	}

	auto MyRenderer::render() -> RenderResult {
		auto pixels = new RGBA[width * height];
		/*for (int i = 0; i < height; i++)
			for (int j = 0; j < width; j++)
				pixels[(height - i - 1) * width + j] = { 0,0,255,1 };*/
		ShaderCreator shaderCreator{};
		for (auto& mtl : scene.materials) {
			shaderPrograms.push_back(shaderCreator.create(mtl, scene.textures));
		}
		//coordinate transform 
		VertexTransformer vertexTransformer{};
		vertexTransformer.exec(spScene);
		//stage 1
		photonshoot();
		//stage 2
		for (int i = 0; i < height; i++) {
			for (int j = 0; j < width; j++) {
				//for every pixel in the camera
				RGB color = Vec3{ 0 };
				for (int s = 0; s < samplenum; s++)
				{
					auto r = defaultSamplerInstance<UniformInSquare>().sample2d();
					auto ray = camera.shoot((float(j) + r.x) / float(width), (float(i) + r.y) / float(height));
					color += trace(ray, 0);
				}
				color /= samplenum;
				color = gamma(color);
				Vec4 itertemp1 = { color, 1 };
				pixels[(height - i - 1) * width + j] = itertemp1;
			}
		}
		/*for (int i = 0; i < height; i++)
			for (int j = 0; j < width; j++)
				pixels[(height - i - 1) * width + j] = { 0,0,255,1 };*/
		return { pixels, width, height };
	}
	//get hitrecord
	tuple<HitRecord, bool> MyRenderer::closestHit(const Ray& r, bool isarea) {
		bool islight = false;
		HitRecord closestHit = nullopt;
		float closest = FLOAT_INF;
		for (auto& s : scene.sphereBuffer) {
			auto hitRecord = Intersection::xSphere(r, s, 0.001, closest);
			if (hitRecord && hitRecord->t < closest) {
				closest = hitRecord->t;
				closestHit = hitRecord;
			}
		}
		for (auto& t : scene.triangleBuffer) {
			auto hitRecord = Intersection::xTriangle(r, t, 0.001, closest);
			if (hitRecord && hitRecord->t < closest) {
				closest = hitRecord->t;
				closestHit = hitRecord;
			}
		}
		for (auto& p : scene.planeBuffer) {
			auto hitRecord = Intersection::xPlane(r, p, 0.000001, closest);
			if (hitRecord && hitRecord->t < closest) {
				closest = hitRecord->t;
				closestHit = hitRecord;
			}
		}
		if (isarea)
			for (auto& i : scene.areaLightBuffer) {
				auto hitRecord = Intersection::xAreaLight(r, i, 0.000001, closest);
				if (hitRecord && hitRecord->t < closest) {
					closest = hitRecord->t;
					closestHit = hitRecord;
					closestHit->normal = i.radiance;
					islight = true;
				}
			}
		return { closestHit,islight };
	}
}
