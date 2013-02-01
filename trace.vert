#version 430

uniform mat4 transform;

in vec3 a_vertex;

out vec2 v_uv;
out vec3 v_ray;

void main()
{
    v_uv   = a_vertex.xy * 0.5 + 0.5;
    v_ray  = (vec4(a_vertex, 1.0) * transform).xyz;

    gl_Position = vec4(a_vertex, 1.0);
}