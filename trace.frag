#version 140

precision highp float;

out vec4 gl_FragColor;

uniform int frame;
uniform int rand;
uniform float accum;
uniform vec3 eye;
uniform vec4 viewport;

uniform  sampler2D hsphere;
uniform  sampler2D lights;

uniform  sampler1D vertices;
uniform  sampler1D colors;
uniform usampler1D indices;

uniform  sampler2D source;


in vec2 v_uv;
in vec3 v_ray;

const vec3 up = vec3(0.0, 1.0, 0.0);

const float EPSILON  = 1e-6;
const float INFINITY = 1e+4;

// intersection with triangle
bool intersection(
	const in vec3  triangle[3]
,	const in vec3  origin
,	const in vec3  ray
,	const in float tm
,   out float t)
{
	vec3 e0 = triangle[1] - triangle[0];
	vec3 e1 = triangle[2] - triangle[0];

	vec3  h = cross(ray, e1);
	float a = dot(e0, h);

	// if(a > -EPSILON && a < EPSILON) // backface culling off
	if(a < EPSILON) // backface culling on
		return false;

	float f = 1.0 / a;

	vec3  s = origin - triangle[0];
	float u = f * dot(s, h);

	if(u < 0.0 || u > 1.0)
		return false;

	vec3  q = cross(s, e0);
	float v = f * dot(ray, q);

	if(v < 0.0 || u + v > 1.0)
		return false;

	t = f * dot(e1, q);

	if (t < EPSILON)
		return false;

	return (t > 0.0) && (t < tm);
}

// plane intersection
bool intersectionPlane(
    const in vec4  plane
,   const in vec3  origin
,   const in vec3  ray
,   const in float tm
,   out float t)
{
    t = -(dot(plane.xyz, origin) + plane.w) / dot(plane.xyz, ray);
    return (t > 0.0) && (t < tm);
}

// sphere intersection
bool intersectionSphere(
    const in vec4  sphere
,   const in vec3  origin
,   const in vec3  ray
,   const in float tm
,   out float t)
{
    bool  r = false;
    vec3  d = origin - sphere.xyz;  // distance

    float b = dot(ray, d);
    float c = dot(d, d) - sphere.w * sphere.w;

    t = b * b - c;

    if(t > 0.0)
    {
        t = -b - sqrt(t);
        r = (t > 0.0) && (t < tm);
    }
    return r;
}

// intersection with scene geometry
float intersection(
    const in vec3 origin
,   const in vec3 ray
,   out vec3 triangle[3]
,   out int index)
{
    float tm = INFINITY;
    float t = INFINITY;

	 vec3 tv[3];
	 vec4 tc;
	ivec4 ti;

	for(int i = 2; i < 34; ++i)
	{
		ti = ivec4(texelFetch(indices, i, 0));

		tv[0] = texelFetch(vertices, ti[0], 0).xyz;
		tv[1] = texelFetch(vertices, ti[1], 0).xyz;
		tv[2] = texelFetch(vertices, ti[2], 0).xyz;

		if(intersection( tv, origin, ray, tm, t))
		{
			triangle = tv;
			index = ti[3];
			tm = t;
		}
	}

    return tm;
}

// intersection with scene geometry
float shadow(
	const in int fragID
,	const in ivec2 lightssize
,	const in vec3 origin
,	const in vec3 n)
{

    float tm = INFINITY;
	float t = INFINITY;

	 vec3 tv[3];
	 vec4 tc;
	ivec4 ti;

	int i = int(mod(fragID, lightssize[0] * lightssize[1]));

    int y = int(i / float(lightssize[0]));
    int x = int(i - y * lightssize[0]);

	// select random point on hemisphere
	vec3 ray = normalize(texelFetch(lights, ivec2(x, y), 0).rgb - origin);

	float a = dot(ray, n);

	if(a < EPSILON)
		return 0.0;

	for(int i = 4; i < 34; ++i)
	{
		ti = ivec4(texelFetch(indices, i, 0));

		tv[0] = texelFetch(vertices, ti[0], 0).xyz;
		tv[1] = texelFetch(vertices, ti[1], 0).xyz;
		tv[2] = texelFetch(vertices, ti[2], 0).xyz;

		if(intersection( tv, origin, ray, tm, t))
			return 0.0;
	}
	return a * a * a;
}

vec3 normal(
	const in vec3 triangle[3]
,	out mat3 tangentspace)
{
	vec3 e0 = triangle[1] - triangle[0];
	vec3 e1 = triangle[2] - triangle[0];

	// hemisphere samplepoints is oriented up

	tangentspace[0] = normalize(e0);
	tangentspace[1] = normalize(cross(e0, e1));
	tangentspace[2] = cross(tangentspace[1], tangentspace[0]);

	return tangentspace[1];
}

vec3 random(
	const in int fragID
,	const in ivec2 hspheresize)
{
	int i = int(mod(fragID, hspheresize[0] * hspheresize[1]));

    int y = int(i / float(hspheresize[0]));
    int x = int(i - y * hspheresize[0]);

	// select random point on hemisphere
	return texelFetch(hsphere, ivec2(x, y), 0).rgb;
}

/*vec4 shade(
	const in vec3 po
,	const in vec3 triangle[3]
,	const in vec4 color
,	const in vec3 ray
,	out vec3 refl)
{
	vec3  n = normal(triangle);

	// 1. absorb some color from 
	// 2. get random direction on oriented disk
	// 3. check for intersection with lights, and if, check for occluder
	// 4. if occluder? get weighted diffuse	

	//refl = reflect(rayd, n);
	//float s = shadow(po, refl) ? 0.5 : 1.0;

	return vec4(n * 0.5 + 0.5, 1.0);
}*/

// http://gpupathtracer.blogspot.de/
// http://www.iquilezles.org/www/articles/simplepathtracing/simplepathtracing.htm
// http://www.cs.dartmouth.edu/~fabio/teaching/graphics08/lectures/18_PathTracing_Web.pdf
// http://undernones.blogspot.de/2010/12/gpu-ray-tracing-with-glsl.html
// http://www.iquilezles.org/www/articles/simplegpurt/simplegpurt.htm
// http://www.lighthouse3d.com/tutorials/maths/ray-triangle-intersection/

void main()
{
    vec3 origin = eye;
    vec3 ray = normalize(v_ray);
	vec3 n;
	mat3 tangentspace;

	// fragment index for random variation
	
	ivec2 hspheresize = textureSize(hsphere, 0);
	ivec2 lightssize = textureSize(lights, 0);

	vec2 xy = v_uv * vec2(viewport[0], viewport[1]);
	int fragID = int(xy.y * viewport[0] + xy.x + frame + rand);


	// triangle data
    vec3 triangle[3];
    vec3 color;
    int index;

	// path color accumulation
	vec3 maskColor = vec3(1.0);
	vec3 pathColor = vec3(0.0);

	float t = INFINITY;

	for(int bounce = 0; bounce < 10; ++bounce)
	{
  		t = intersection(origin, ray, triangle, index); // compute t from objects

		// TODO: break on no intersection, with correct path color weight?
		if(t == INFINITY)
			break;

		origin = origin + ray * t;
		n = normal(triangle, tangentspace);

  		vec3 color = texelFetch(colors, index, 0).xyz; // compute material color from hit
  		float lighting = shadow(fragID + bounce, lightssize, origin, n) * 0.2; // compute direct lighting from hit

  		// accumulate incoming light
  		maskColor *= color;
  		pathColor += maskColor * lighting;

  		ray = tangentspace * random(fragID + bounce, hspheresize); // compute next ray
	}

    /*
    float t;
	vec3 po = rayo;

	t = intersection(rayo, rayd, triangle, color);

	po += rayd * t;
	color = shade(po, triangle, color, rayd, refl);
    */
    //gl_FragColor = texture(source, v_uv) + 0.001 * color; //v_uv.x, v_uv.y, 0.0, 1.0);
    
    gl_FragColor = vec4(mix(pathColor, texture(source, v_uv).rgb, accum), 1.0);
    //gl_FragColor = vec4(vec3(accum * 0.5), 1.0);
}