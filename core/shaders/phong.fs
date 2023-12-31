#version 330 core
in highp vec3 FragPos;
in highp vec3 Normal;
in highp vec3 myColor;

uniform vec3 viewPos;
struct Material {
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
	float shininess;
};

struct Light {
	vec3 position;
	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};

uniform Light light;
uniform Material material;

void main()
{
	vec3 ambient=light.ambient*material.ambient;
	vec3 norm=normalize(Normal);
	vec3 lightDir=normalize(light.position-FragPos);
	float diff = max(dot(norm, lightDir), 0.0);
	vec3 diffuse = material.diffuse*diff * light.diffuse;
	vec3 viewDir=normalize(viewPos-FragPos);
	vec3 reflectDir=reflect(-lightDir,Normal);
	float spec=max(dot(reflectDir,viewDir),0.0);
	spec=pow(spec,material.shininess);
	vec3 result = (ambient + diffuse+material.specular*spec*light.specular) * myColor;
	gl_FragColor = vec4(result, 1.0);
}