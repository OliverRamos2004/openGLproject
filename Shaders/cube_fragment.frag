#version 410 core

out vec4 FragColor;

in vec3 Normal;
in vec3 FragPos;
in vec2 TexCoords;

uniform float exposure;
uniform float envMix;   // 0 = no skybox reflection, 1 = full
uniform bool unlit;




// ---------------------------------------------------------------------
// Structs (same layout as in your C++ config)
// ---------------------------------------------------------------------
struct Material {
    sampler2D diffuse;
    sampler2D specular;
    float shininess;
    float alpha;
};

uniform Material material;

struct DirLight {
    vec3 direction;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct PointLight {
    vec3 position;

    float constant;
    float linear;
    float quadratic;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct SpotLight {
    vec3 position;
    vec3 direction;
    float cutOff;
    float outerCutOff;

    float constant;
    float linear;
    float quadratic;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

// ---------------------------------------------------------------------
// Light arrays + counters  (match C++: applyDirLights / applyPointLights / applySpotLights)
// ---------------------------------------------------------------------
#define MAX_DIR_LIGHTS   4
#define MAX_POINT_LIGHTS 8
#define MAX_SPOT_LIGHTS  4

uniform int numDirLights;
uniform int numPointLights;
uniform int numSpotLights;

uniform DirLight   dirLights[MAX_DIR_LIGHTS];
uniform PointLight pointLights[MAX_POINT_LIGHTS];
uniform SpotLight  spotLights[MAX_SPOT_LIGHTS];

uniform vec3 viewPos;
uniform samplerCube skybox;
uniform samplerCube pointShadowMap;
uniform float farPlane;


// ---------------------------------------------------------------------
// Function declarations
// ---------------------------------------------------------------------
vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir);
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir);
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir);
float PointShadowCalculation(vec3 fragPos, vec3 lightPos);

// (optional depth helper – unused here)
float near = 0.1;
float far  = 100.0;
float LinearizeDepth(float depth)
{
    float z = depth * 2.0 - 1.0; // back to NDC
    return (2.0 * near * far) / (far + near - z * (far - near));
}

// ---------------------------------------------------------------------
// main: accumulate all lights, then apply alpha from Material
// ---------------------------------------------------------------------
void main()
{
    vec3 norm    = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);

    vec3 result = vec3(0.0);

    vec3 albedo = texture(material.diffuse, TexCoords).rgb;

    // If this object is "unlit" (paintings), just show the texture color.
    if (unlit) {
        FragColor = vec4(albedo, material.alpha);
        return;
    }



    // Directional lights
    for (int i = 0; i < numDirLights; ++i) {
        result += CalcDirLight(dirLights[i], norm, viewDir);
    }

    // Point lights
    for (int i = 0; i < numPointLights; ++i) {
        result += CalcPointLight(pointLights[i], norm, FragPos, viewDir);
    }

    // Spot lights
    for (int i = 0; i < numSpotLights; ++i) {
        result += CalcSpotLight(spotLights[i], norm, FragPos, viewDir);
    }

    // ... previous lighting calculation
    // 2) Environment reflection from cubemap
    //    viewDir points from fragment -> camera, so we use reflect(-viewDir, N).
    vec3 R = reflect(-viewDir, norm);
    vec3 envColor = texture(skybox, R).rgb;

    // You're feeding roughness into material.specular right now.
    // Rough walls should reflect LESS, so invert roughness -> reflectionMask.
    float rough = texture(material.specular, TexCoords).r;
    float reflectionMask = (1.0 - rough) * envMix;

    vec3 finalColor = mix(result, envColor, reflectionMask);


    // exposure + gamma MUST be last
    finalColor = vec3(1.0) - exp(-finalColor * exposure);
    finalColor = pow(finalColor, vec3(1.0 / 2.2));

    FragColor = vec4(finalColor, material.alpha);

}

// ---------------------------------------------------------------------
// Light calculations (unchanged except they use material + TexCoords)
// ---------------------------------------------------------------------
vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir)
{
    vec3 lightDir = normalize(-light.direction);
    float diff = max(dot(normal, lightDir), 0.0);

    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);

    vec3 texDiffuse = vec3(texture(material.diffuse, TexCoords));
    vec3 texSpec    = vec3(texture(material.specular, TexCoords));

    vec3 ambient  = light.ambient  * texDiffuse;
    vec3 diffuse  = light.diffuse  * diff * texDiffuse;
    vec3 specular = light.specular * spec * texSpec;

    return ambient + diffuse + specular;
}

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);
    float diff = max(dot(normal, lightDir), 0.0);

    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);

    float distance    = length(light.position - fragPos);
    float attenuation = 1.0 /
        (light.constant +
         light.linear * distance +
         light.quadratic * (distance * distance));

    vec3 texDiffuse = vec3(texture(material.diffuse, TexCoords));
    vec3 texSpec    = vec3(texture(material.specular, TexCoords));

    vec3 ambient  = light.ambient  * texDiffuse;
    vec3 diffuse  = light.diffuse  * diff * texDiffuse;
    vec3 specular = light.specular * spec * texSpec;

    ambient  *= attenuation;
    diffuse  *= attenuation;
    specular *= attenuation;

    //return ambient + diffuse + specular;
     // Apply point light shadow
    float shadow = PointShadowCalculation(fragPos, light.position);

    // Ambient stays unshadowed
    return ambient + (1.0 - shadow) * (diffuse + specular);

}

vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);
    float diff = max(dot(normal, lightDir), 0.0);

    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);

    float distance    = length(light.position - fragPos);
    float attenuation = 1.0 /
        (light.constant +
         light.linear * distance +
         light.quadratic * (distance * distance));

    float theta   = dot(lightDir, normalize(-light.direction));
    float epsilon = light.cutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);

    vec3 texDiffuse = vec3(texture(material.diffuse, TexCoords));
    vec3 texSpec    = vec3(texture(material.specular, TexCoords));

    vec3 ambient  = light.ambient  * texDiffuse;
    vec3 diffuse  = light.diffuse  * diff * texDiffuse;
    vec3 specular = light.specular * spec * texSpec;

    ambient  *= attenuation * intensity;
    diffuse  *= attenuation * intensity;
    specular *= attenuation * intensity;

    return ambient + diffuse + specular;
}

float PointShadowCalculation(vec3 fragPos, vec3 lightPos)
{
    // Distance from light to fragment
    vec3 lightToFrag = fragPos - lightPos;
    float dist = length(lightToFrag);

    // Cubemap sampling direction MUST match depth pass
    float closest = texture(pointShadowMap, lightToFrag).r * farPlane;

    // Shadow bias (prevent acne)
    float bias = 0.03;

    // Shadow test
    return (dist - bias > closest) ? 1.0 : 0.0;
}

