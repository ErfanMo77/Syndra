// Basic diffuse Shader
#type vertex

#version 460 core
	
layout(location = 0) in vec3 a_pos;
layout(location = 1) in vec2 a_uv;
layout(location = 2) in vec3 a_normal;
layout(location = 3) in int a_ID;

uniform mat4 u_ViewProjection;
uniform mat4 u_trans;

out vec3 v_pos;
out vec2 v_uv;
out vec3 v_normal;
out flat int v_ID;

void main(){
	v_normal = mat3(transpose(inverse(u_trans)))*a_normal;
	v_pos = vec3(u_trans*vec4(a_pos,1.0));
	v_uv = a_uv;
	v_ID = a_ID;
	gl_Position = u_ViewProjection * u_trans *vec4(a_pos,1.0);
}

#type fragment
#version 460 core
layout(location = 0) out vec4 fragColor;	
layout(location = 1) out int id;	

uniform sampler2D u_Texture;
uniform vec3 cubeCol;

uniform vec3 cameraPos;
uniform vec3 lightPos;

in vec3 v_pos;
in vec2 v_uv;
in vec3 v_normal;
in flat int v_ID;

void main(){	
    vec3 norm = normalize(v_normal);
	vec3 lightDir = normalize(lightPos - v_pos);
	vec3 viewDir = normalize(cameraPos - v_pos);
	vec3 reflectDir = reflect(-lightDir,norm);

	float diff = max(dot(norm,lightDir),0);
	vec3 color = vec3(texture(u_Texture,v_uv));
	vec3 result = (diff+0.2)*color;
	
	id = v_ID;
	fragColor = vec4(result * cubeCol,1.0);
}

