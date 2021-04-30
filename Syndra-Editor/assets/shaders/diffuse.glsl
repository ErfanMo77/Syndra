// Basic diffuse Shader
#type vertex

#version 450 core
	
layout(location = 0) in vec3 a_pos;
layout(location = 1) in vec2 a_uv;
layout(location = 2) in vec3 a_normal;
layout(location = 3) in vec3 a_tangent;
layout(location = 4) in vec3 a_bitangent;


uniform mat4 u_ViewProjection;
uniform mat4 u_trans;

out vec3 v_pos;
out vec2 v_uv;
out vec3 v_normal;

void main(){
	v_normal = mat3(transpose(inverse(u_trans)))*a_normal;
	v_pos = vec3(u_trans*vec4(a_pos,1.0));
	v_uv = a_uv;
	gl_Position = u_ViewProjection * u_trans *vec4(a_pos,1.0);
}

#type fragment
#version 450 core
layout(location = 0) out vec4 fragColor;	

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;

uniform vec3 cubeCol;

uniform vec3 cameraPos;
uniform vec3 lightPos;

in vec3 v_pos;
in vec2 v_uv;
in vec3 v_normal;

void main(){	
    vec3 norm = normalize(v_normal);
	vec3 lightDir = normalize(lightPos - v_pos);
	vec3 viewDir = normalize(cameraPos - v_pos);
	vec3 reflectDir = reflect(-lightDir,norm);

	float diff = max(dot(norm,lightDir),0);

	float spec = pow(max(dot(viewDir, reflectDir), 0.0), 16);

	vec3 color = texture(texture_diffuse1,v_uv).rgb;

	vec3 specular = spec * texture(texture_specular1,v_uv).rgb;
	vec3 diffuse = diff * color;

	vec3 result = diffuse + specular + vec3(0.05);
	if(color == vec3(0)){
		result = vec3(diff);
	}

	fragColor = vec4(result,1.0);
}

