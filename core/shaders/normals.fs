// use normal as colour
#version 330
in highp vec3 FragPos;
in highp vec3 Normal;
in highp vec3 myColor;
out highp vec4 FragColor;
void main() {
    FragColor = vec4(Normal, 1.0);
}