#version 430

uniform int frame;
uniform float accum;
uniform vec3 eye;
uniform vec4 viewport;

uniform  sampler2D hsphere;

uniform  sampler1D vertices;
uniform  sampler1D colors;
uniform usampler1D indices;

uniform  sampler2D source;


in vec2 v_uv;
in vec3 v_ray;

const float EPSILON  = 1e-6;
const float INFINITY = 1e+4;

// intersection with triangle
bool intersection(
	const in vec3  tri0
,	const in vec3  tri1
,	const in vec3  tri2
,	const in vec3  rayo
,	const in vec3  rayd
,	const in float tm
,   out float t)
{
	vec3 e0 = tri1 - tri0;
	vec3 e1 = tri2 - tri0;

	vec3  h = cross(rayd, e1);
	float a = dot(e0, h);

	if(a > -EPSILON && a < EPSILON)
		return false;

	float f = 1.0 / a;

	vec3  s = rayo - tri0;
	float u = f * dot(s, h);

	if(u < 0.0 || u > 1.0)
		return false;

	vec3  q = cross(s, e0);
	float v = f * dot(rayd, q);

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
,   const in vec3  rayo
,   const in vec3  rayd
,   const in float tm
,   out float t)
{
    t = -(dot(plane.xyz, rayo) + plane.w) / dot(plane.xyz, rayd);
    return (t > 0.0) && (t < tm);
}

// sphere intersection
bool intersectionSphere(
    const in vec4  sphere
,   const in vec3  rayo
,   const in vec3  rayd
,   const in float tm
,   out float t)
{
    bool  r = false;
    vec3  d = rayo - sphere.xyz;  // distance

    float b = dot(rayd, d);
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
    const in vec3 rayo
,   const in vec3 rayd
,   out vec3 triangle[3]
,   out vec4 color)
{
    float tm = INFINITY;
    float t;

	color = vec4(0.0, 0.0, 0.0, 1.0);

	 vec3 tv[3];
	 vec4 tc;
	ivec4 ti;

	for(int i = 2; i < 32; ++i)
	{
		ti = ivec4(texelFetch(indices, i, 0));

		tv[0] = texelFetch(vertices, ti[0], 0).xyz;
		tv[1] = texelFetch(vertices, ti[1], 0).xyz;
		tv[2] = texelFetch(vertices, ti[2], 0).xyz;

		if(intersection( tv[0], tv[1], tv[2], rayo, rayd, tm, t))
		{
			triangle = tv;
			color = texelFetch(colors, ti[3], 0);
			tm = t;
		}
	}
    return tm;
}

// intersection with scene geometry
bool shadow(
    const in vec3 rayo
,   const in vec3 rayd)
{
    float tm = INFINITY;
	float t;

	 vec3 tv[3];
	 vec4 tc;
	ivec4 ti;

	// check for intersections with light triangles

	for(int i = 0; i < 2; ++i)
	{
		ti = ivec4(texelFetch(indices, i, 0));

		tv[0] = texelFetch(vertices, ti[0], 0).xyz;
		tv[1] = texelFetch(vertices, ti[1], 0).xyz;
		tv[2] = texelFetch(vertices, ti[2], 0).xyz;

		if(intersection( tv[0], tv[1], tv[2], rayo, rayd, tm, t))
		{
			tm = t;
			continue;
		}
	}
	if(tm >= INFINITY)
		return false;

	for(int i = 2; i < 32; ++i)
	{
		ti = ivec4(texelFetch(indices, i, 0));

		tv[0] = texelFetch(vertices, ti[0], 0).xyz;
		tv[1] = texelFetch(vertices, ti[1], 0).xyz;
		tv[2] = texelFetch(vertices, ti[2], 0).xyz;

		if(intersection( tv[0], tv[1], tv[2], rayo, rayd, tm, t))
			return false;
	}
	return true;
}

vec3 normal(
	const in vec3 tri[3])
{
	vec3 e0 = tri[1] - tri[0];
	vec3 e1 = tri[2] - tri[0];

	return normalize(cross(e0, e1));
}


vec4 shade(
	const in vec3 po
,	const in vec3 tri[3]
,	const in vec4 color
,	const in vec3 rayd
,	out vec3 refl)
{
	vec3  n = normal(tri);

	// 1. absorb some color from 
	// 2. get random direction on oriented disk
	// 3. check for intersection with lights, and if, check for occluder
	// 4. if occluder? get weighted diffuse	

	//refl = reflect(rayd, n);
	//float s = shadow(po, refl) ? 0.5 : 1.0;

	return vec4(n * 0.5 + 0.5, 1.0);
}

// http://gpupathtracer.blogspot.de/
// http://www.iquilezles.org/www/articles/simplepathtracing/simplepathtracing.htm
// http://www.cs.dartmouth.edu/~fabio/teaching/graphics08/lectures/18_PathTracing_Web.pdf
// http://undernones.blogspot.de/2010/12/gpu-ray-tracing-with-glsl.html
// http://www.iquilezles.org/www/articles/simplegpurt/simplegpurt.htm
// http://www.lighthouse3d.com/tutorials/maths/ray-triangle-intersection/

void main()
{
    vec3 rayo = eye;
    vec3 rayd = normalize(v_ray);

    vec3 triangle[3];
    vec4 color;
	vec3 refl;
    /*
    float t;
	vec3 po = rayo;

	t = intersection(rayo, rayd, triangle, color);

	po += rayd * t;
	color = shade(po, triangle, color, rayd, refl);
    */
    //gl_FragColor = texture(source, v_uv) + 0.001 * color; //v_uv.x, v_uv.y, 0.0, 1.0);

    
    vec2 xy = v_uv * vec2(viewport[0], viewport[1]);
    ivec2 wh = textureSize(hsphere, 0);

    int i = int(mod(xy.y * viewport[0] + xy.x + frame, wh[0] * wh[1]));

    int y = int(i / float(wh[0]));
    int x = int(i - y * wh[0]);

    // make randome oriented hemisphere function!

    gl_FragColor = mix(texelFetch(hsphere, ivec2(x, y), 0), texture(source, v_uv), accum);
    //gl_FragColor = vec4(vec3(accum * 0.5), 1.0);
}