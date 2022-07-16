#version 430 core

#define MAX_OBJECT_COUNT 64
#define MAX_LIGHT_COUNT 4

#define RENDER_DISTANCE 10000
#define EPSILON 0.0001
#define PI 3.1415926538
#define OUTLINE_WIDTH 0.004
#define OUTLINE_COLOR vec4(1.0, 0.0, 1.0, 1.0)

in vec2 fragUV;
out vec4 fragColor;

struct Ray {
	vec3 origin;
	vec3 direction;
};

struct Material {
	vec3 albedo;
	vec3 specular;
	vec3 emission;
	float emissionStrength;
	float roughness;
	float specularHighlight;
	float specularExponent;
};

struct SurfacePoint {
	vec3 position;
	vec3 normal;
	Material material;
};

struct Object {
	uint type;
	vec3 position;
	vec3 scale;
	Material material;
};

struct PointLight {
	vec3 position;
	float radius;
	vec3 color;
	float power;
	float reach; // Only points within this distance of the light will be affected
};

uniform sampler2D u_screenTexture;
uniform sampler2D u_skyboxTexture;
uniform int u_accumulatedPasses; // How many passes have been added to the texture
uniform bool u_directOutputPass; // If this is true, the shader will draw the input texture directly to the screen. (Used to draw the contents of the FBO to the screen)
uniform float u_time;
uniform vec3 u_cameraPosition;
uniform mat4 u_rotationMatrix;
uniform float u_aspectRatio;
uniform bool u_debugKeyPressed;

uniform int u_shadowResolution;
uniform int u_lightBounces;
uniform int u_framePasses;
uniform float u_blur;
uniform float u_bloomRadius;
uniform float u_bloomIntensity;
uniform float u_skyboxStrength;
uniform float u_skyboxGamma;
uniform float u_skyboxCeiling;
uniform Object u_objects[MAX_OBJECT_COUNT];
uniform PointLight u_lights[MAX_LIGHT_COUNT];
uniform bool u_planeVisible;
uniform Material u_planeMaterial;

uniform int u_selectedSphereIndex;

float rand(vec2 co){
    return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}

bool sphereIntersection(vec3 position, float radius, Ray ray, out float hitDistance){
    float t = dot(position - ray.origin, ray.direction);
	vec3 p = ray.origin + ray.direction * t;

	float y = length(position - p);
	if (y < radius) { 
		float x =  sqrt(radius*radius - y*y);
		float t1 = t-x;
		if (t1 >  0) {
			hitDistance = t1;
			return true;
		}

	}
	
	return false;
}

bool boxIntersection(vec3 position, vec3 size, Ray ray, out float hitDistance) {
	float t1 = -1000000000000.0;
    float t2 = 1000000000000.0;

	vec3 boxMin = position - size / 2.0;
	vec3 boxMax = position + size / 2.0;

    vec3 t0s = (boxMin - ray.origin) / ray.direction;
    vec3 t1s = (boxMax - ray.origin) / ray.direction;

    vec3 tsmaller = min(t0s, t1s);
    vec3 tbigger = max(t0s, t1s);

    t1 = max(t1, max(tsmaller.x, max(tsmaller.y, tsmaller.z)));
    t2 = min(t2, min(tbigger.x, min(tbigger.y, tbigger.z)));

	hitDistance = t1;

    return t1 >= 0 && t1 <= t2;
}

vec3 boxNormal(vec3 cubePosition, vec3 size, vec3 surfacePosition)
{
    // Source: https://gist.github.com/Shtille/1f98c649abeeb7a18c5a56696546d3cf
    // step(edge,x) : x < edge ? 0 : 1

	vec3 boxMin = cubePosition - size / 2.0;
	vec3 boxMax = cubePosition + size / 2.0;

	vec3 center = (boxMax + boxMin) * 0.5;
	vec3 boxSize = (boxMax - boxMin) * 0.5;
	vec3 pc = surfacePosition - center;
	// step(edge,x) : x < edge ? 0 : 1
	vec3 normal = vec3(0.0);
	normal += vec3(sign(pc.x), 0.0, 0.0) * step(abs(abs(pc.x) - boxSize.x), EPSILON);
	normal += vec3(0.0, sign(pc.y), 0.0) * step(abs(abs(pc.y) - boxSize.y), EPSILON);
	normal += vec3(0.0, 0.0, sign(pc.z)) * step(abs(abs(pc.z) - boxSize.z), EPSILON);
	return normalize(normal);
}

bool planeIntersection(vec3 planeNormal, vec3 planePoint, Ray ray, out float hitDistance) 
{ 
    float denom = dot(planeNormal, ray.direction); 
    if (abs(denom) > EPSILON) { 
        vec3 d = planePoint - ray.origin; 
        hitDistance = dot(d, planeNormal) / denom; 
        return (hitDistance >= EPSILON); 
    } 
 
    return false; 
} 

bool raycast(Ray ray, out SurfacePoint hitPoint) {
	bool didHit = false;
	float minHitDist = RENDER_DISTANCE;

	float hitDist;
	for (int i = 0; i<u_objects.length(); i++) {
		if (u_objects[i].type == 0) continue;

		if (u_objects[i].type == 1 && sphereIntersection(u_objects[i].position, u_objects[i].scale.x, ray, hitDist)) {
			didHit = true;
			if (hitDist < minHitDist) {
				minHitDist = hitDist;
				hitPoint.position = ray.origin + ray.direction * minHitDist;
				hitPoint.normal = normalize(hitPoint.position - u_objects[i].position);
				hitPoint.material = u_objects[i].material;
			}
		}

		if (u_objects[i].type == 2 && boxIntersection(u_objects[i].position, u_objects[i].scale, ray, hitDist)) {
			didHit = true;
			if (hitDist < minHitDist) {
				minHitDist = hitDist;
				hitPoint.position = ray.origin + ray.direction * minHitDist;
				hitPoint.normal = boxNormal(u_objects[i].position, u_objects[i].scale, ray.origin + ray.direction * minHitDist);
				hitPoint.material = u_objects[i].material;
			}
		}
	}

	if (u_planeVisible && planeIntersection(vec3(0,1,0), vec3(0, 0, 0), ray, hitDist)) {
		didHit = true;
		if (hitDist < minHitDist) {
			minHitDist = hitDist;
			hitPoint.position = ray.origin + ray.direction * minHitDist;
			hitPoint.normal = vec3(0,1,0);
			hitPoint.material = u_planeMaterial;
		}
	}

	return didHit;
}

// Adapted from https://bitbucket.org/Daerst/gpu-ray-tracing-in-unity/src/Tutorial_Pt2/Assets/RayTracingShader.compute
mat3x3 getTangentSpace(vec3 normal)
{
    // Choose a helper vector for the cross product
    vec3 helper = vec3(1, 0, 0);
    if (abs(normal.x) > 0.99)
        helper = vec3(0, 0, 1);

    // Generate vectors
    vec3 tangent = normalize(cross(normal, helper));
    vec3 binormal = normalize(cross(normal, tangent));
    return mat3x3(tangent, binormal, normal);
}

// Basic rejection sampling method
vec3 _sampleHemisphere(vec3 normal, vec2 seed)
{
    vec3 vec = normalize(vec3(rand(seed)*2.0-1.0,rand(seed.yx+vec2(1.123123123,2.545454))*2.0-1.0,rand(seed-vec2(9.21428,7.43163431))*2.0-1.0));
	if (dot(vec, normal) < 0.0) vec *= -1; 

	return vec;
}

// Adapted from https://bitbucket.org/Daerst/gpu-ray-tracing-in-unity/src/Tutorial_Pt2/Assets/RayTracingShader.compute
vec3 sampleHemisphere(vec3 normal, float alpha, vec2 seed)
{
    // Sample the hemisphere, where alpha determines the kind of the sampling
    float cosTheta = pow(rand(seed), 1.0 / (alpha + 1.0));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
    float phi = 2 * PI * rand(seed.yx);
    vec3 tangentSpaceDir = vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);

    // Transform direction to world space
    return getTangentSpace(normal) * tangentSpaceDir;
}

vec3 sampleSkybox(vec3 dir) {
	if (u_skyboxStrength == 0.0) return vec3(0.0);
	
	return min(vec3(u_skyboxCeiling), u_skyboxStrength*pow(texture(u_skyboxTexture, vec2(0.5 + atan(dir.x, dir.z)/(2*PI), 0.5 + asin(-dir.y)/PI)).xyz, vec3(1.0/u_skyboxGamma)));
}

// Adds up the total light received directly from all light sources
vec3 computeDirectIllumination(SurfacePoint point, vec3 observerPos, float seed) {
	vec3 directIllumination = vec3(0);

	for (int lightIndex = 0; lightIndex<u_lights.length(); lightIndex++) {
		PointLight light = u_lights[lightIndex];
			
		float lightDistance = length(light.position - point.position);
		if (lightDistance > light.reach) continue;

		float diffuse = clamp(dot(point.normal, normalize(light.position-point.position)), 0.0, 1.0);

		if (diffuse > EPSILON || point.material.roughness < 1.0) {
			// Shadow raycasting
			int shadowRays = int(u_shadowResolution*light.radius*light.radius/(lightDistance*lightDistance)+1); // There must be a better way to find the right amount of shadow rays
			int shadowRayHits = 0;
			for (int i = 0; i<shadowRays; i++) {
				// Sample a point on the light sphere
				
				vec3 lightSurfacePoint = light.position + normalize(vec3(rand(vec2(i+seed, 1)+point.position.xy), rand(vec2(i+seed, 2)+point.position.yz), rand(vec2(i+seed, 3)+point.position.xz))) * light.radius;
				vec3 lightDir = normalize(lightSurfacePoint - point.position);
				vec3 rayOrigin = point.position + lightDir * EPSILON * 2.0;
				float maxRayLength = length(lightSurfacePoint - rayOrigin);
				Ray shadowRay = Ray(rayOrigin, lightDir);
				SurfacePoint SR_hit;
				if (raycast(shadowRay, SR_hit)) {
					if (length(SR_hit.position-rayOrigin) < maxRayLength) {
						shadowRayHits += 1;
					}
				}

			}

			// Diffuse 
			float attenuation = lightDistance * lightDistance;
			directIllumination += light.color * light.power * diffuse * point.material.albedo * (1.0-float(shadowRayHits)/shadowRays) / attenuation;
		
			// Specular highlight
			vec3 lightDir = normalize(point.position - light.position);
			vec3 reflectedLightDir = reflect(lightDir, point.normal);
			vec3 cameraDir = normalize(observerPos - point.position);
			directIllumination += point.material.specularHighlight * light.color * (light.power/(lightDistance*lightDistance)) * pow(max(dot(cameraDir, reflectedLightDir), 0.0), 1.0/max(point.material.specularExponent, EPSILON));
	
		}
	}

	return directIllumination;
}

// Based on https://bitbucket.org/Daerst/gpu-ray-tracing-in-unity/src/Tutorial_Pt2/Assets/RayTracingShader.compute
vec3 computeSceneColor(Ray cameraRay, float seed) {
	vec3 totalIllumination = vec3(0);
	vec3 rayOrigin = cameraRay.origin;
	vec3 rayDirection = cameraRay.direction;
	vec3 energy = vec3(1.0);
	for (int depth = 0; depth < u_lightBounces; depth++) {
		SurfacePoint hitPoint;
		if (raycast(Ray(rayOrigin, rayDirection), hitPoint)) {
			// Part one: Hit object's emission
			totalIllumination += energy * hitPoint.material.emission * hitPoint.material.emissionStrength;

			// Part two: Direct light (received directly from light sources)
			totalIllumination += energy * computeDirectIllumination(hitPoint, rayOrigin, seed);

			// Part three: Indirect light (other objects + skybox)
			float specChance = dot(hitPoint.material.specular, vec3(1.0/3.0));
			float diffChance = dot(hitPoint.material.albedo, vec3(1.0/3.0));

			float sum = specChance + diffChance;
			specChance /= sum;
			diffChance /= sum;

			// Roulette-select the ray's path
			float roulette = rand(hitPoint.position.zx+vec2(hitPoint.position.y)+vec2(seed, depth));
			if (roulette < specChance)
			{
				// Specular reflection
				float smoothness = 1.0-hitPoint.material.roughness;
				float alpha = pow(1000.0, smoothness*smoothness);
				if (smoothness == 1.0) {
					rayDirection = reflect(rayDirection, hitPoint.normal);
				} else {
					rayDirection = sampleHemisphere(reflect(rayDirection, hitPoint.normal), alpha, hitPoint.position.zx+vec2(hitPoint.position.y)+vec2(seed, depth));
				}
				rayOrigin = hitPoint.position + rayDirection * EPSILON;
				float f = (alpha + 2) / (alpha + 1);
				energy *= hitPoint.material.specular * clamp(dot(hitPoint.normal, rayDirection) * f, 0.0, 1.0);
			}
			else if (diffChance > 0 && roulette < specChance + diffChance)
			{
				// Diffuse reflection
				rayOrigin = hitPoint.position + hitPoint.normal * EPSILON;
				rayDirection = sampleHemisphere(hitPoint.normal, 1.0, hitPoint.position.zx+vec2(hitPoint.position.y)+vec2(seed, depth));
				energy *= hitPoint.material.albedo * clamp(dot(hitPoint.normal, rayDirection), 0.0, 1.0);
			} else {
				// This means both the hit material's albedo and specular are totally black, so there won't be anymore light. We can stop here.
				break;
			}
		} else {
			// The ray didn't hit anything, so we add the sky's color and we're done
			totalIllumination += energy * sampleSkybox(rayDirection);
			break;
		}
	}

	return totalIllumination;
}

void main() {
	vec2 centeredUV = (fragUV * 2 - vec2(1)) * vec2(u_aspectRatio, 1.0);

	if (u_directOutputPass) {
		vec3 rayDir = (normalize(vec4(centeredUV, -1.0, 0.0)) * u_rotationMatrix).xyz;
		Ray cameraRay = Ray(u_cameraPosition, rayDir);

		fragColor = texture(u_screenTexture, fragUV);
		float divider = float(u_accumulatedPasses);
		fragColor.x /= divider;
		fragColor.y /= divider;
		fragColor.z /= divider;

		// Selected object outline rendering
		if (u_selectedSphereIndex >= 0) {
			float hitDist;

			float selectedSphereDist = length(u_objects[u_selectedSphereIndex].position - u_cameraPosition);

			// Check if this camera ray is hitting the outline
			Object selectedObject = u_objects[u_selectedSphereIndex];
			if (selectedObject.type == 1 && sphereIntersection(selectedObject.position, selectedObject.scale[0]+OUTLINE_WIDTH*selectedSphereDist, cameraRay, hitDist)) {
				if (!sphereIntersection(selectedObject.position, selectedObject.scale[0], cameraRay, hitDist)) {
					fragColor = OUTLINE_COLOR;
				}
			} else if (selectedObject.type == 2 && boxIntersection(selectedObject.position, selectedObject.scale+vec3(OUTLINE_WIDTH*selectedSphereDist), cameraRay, hitDist)) {
				if (!boxIntersection(selectedObject.position, selectedObject.scale, cameraRay, hitDist)) {
					fragColor = OUTLINE_COLOR;
				}
			}
		}
	} else {
		if (u_blur > 0.0 && u_accumulatedPasses > 0) centeredUV += vec2(rand(vec2(1, u_time)+fragUV.xy)*u_blur-u_blur/2, rand(vec2(2, u_time)+fragUV.yx)*u_blur-u_blur/2);
		vec3 rayDir = (normalize(vec4(centeredUV, -1.0, 0.0)) * u_rotationMatrix).xyz;
		Ray cameraRay = Ray(u_cameraPosition, rayDir);

		// Camera raycasting
		vec3 colorSum = computeSceneColor(cameraRay, u_time);
		for (int i = 0; i<u_framePasses-1; i++) colorSum += computeSceneColor(cameraRay, u_time+i);
		fragColor = vec4(colorSum / u_framePasses, 1.0);


		if (u_accumulatedPasses > 0) {
			// Bloom
			SurfacePoint hitPoint;
			vec3 offsetDirection = cameraRay.direction + vec3(rand(vec2(1, u_time)+fragUV)*u_bloomRadius-u_bloomRadius/2, rand(vec2(2, u_time)+fragUV)*u_bloomRadius-u_bloomRadius/2, rand(vec2(3, u_time)+fragUV)*u_bloomRadius-u_bloomRadius/2);
			if (raycast(Ray(cameraRay.origin, offsetDirection), hitPoint)) {
				fragColor += vec4(hitPoint.material.emission*hitPoint.material.emissionStrength*u_bloomIntensity, 1.0);
			}

			// Add last frame back (progressive sampling)
			fragColor += texture(u_screenTexture, fragUV);
		}
	}
}