#version 330 core
layout(location=0) in highp vec3 aPos;
layout(location=1) in highp vec3 aColor;
layout(location=2) in highp vec3 aNormal;

uniform mat4 view;
uniform mat4 proj;
uniform mat4 model;

out highp vec3 FragPos;
out highp vec3 Normal;
out highp vec3 myColor;
void main() {
	FragPos = vec3(model * vec4(aPos, 1.0));
	gl_Position = proj * view * model * vec4(aPos, 1.0);
	Normal = normalize(mat3(transpose(inverse(model))) * aNormal);
	myColor = aColor;
}