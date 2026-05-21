#shader vertex

layout(location = 0) in vec3 Position;

layout(binding = 0) uniform Matrices {
    mat4 view;
    mat4 projection;
    vec4 viewPos;
} u_Matrices;

layout(binding = 1) uniform Model {
    mat4 model;
} u_Model;

void main()
{
    gl_Position = u_Matrices.projection * u_Matrices.view * u_Model.model * vec4(Position, 1.0);
}

#shader fragment

layout(binding = 5) uniform Color {
    vec4 color;
} model;

layout(location = 0) out vec4 FragColor;

void main()
{
    FragColor = model.color;
}