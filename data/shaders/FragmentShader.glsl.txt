#version 460 core

out vec4 FragColor;
in vec3 vLocalPos;

// Binding 0: RGBA8 (RGB=Normal, A=Density)
layout(binding = 0) uniform sampler3D u_VolumeTexture;
// Binding 1: Transfer Function
layout(binding = 1) uniform sampler1D u_TransferFunction;

uniform vec3 u_CameraPos;
uniform mat4 u_ModelMatrix;
uniform vec3 u_LightDir;

uniform float u_StepSize;
uniform float u_DensityThreshold;
uniform float u_OpacityMultiplier;

// Уменьшим макс. шаги для защиты от зависания TDR
const int MAX_STEPS = 512; 

// Jittering (случайный сдвиг старта луча для скрытия слоев)
float random(vec2 co) {
    return fract(sin(dot(co.xy, vec2(12.9898, 78.233))) * 43758.5453);
}

vec2 intersectAABB(vec3 rayOrigin, vec3 rayDir, vec3 boxMin, vec3 boxMax) {
    vec3 tMin = (boxMin - rayOrigin) / rayDir;
    vec3 tMax = (boxMax - rayOrigin) / rayDir;
    vec3 t1 = min(tMin, tMax);
    vec3 t2 = max(tMin, tMax);
    float tNear = max(max(t1.x, t1.y), t1.z);
    float tFar = min(min(t2.x, t2.y), t2.z);
    return vec2(tNear, tFar);
}

void main()
{
    mat4 invModel = inverse(u_ModelMatrix);
    vec3 localCamPos = vec3(invModel * vec4(u_CameraPos, 1.0));
    vec3 rayDir = normalize(vLocalPos - localCamPos);
    
    // Пересечение с кубом (-1..1)
    vec2 tHit = intersectAABB(localCamPos, rayDir, vec3(-1.0), vec3(1.0));
    float tNear = tHit.x;
    float tFar = tHit.y;
    
    if (tNear > tFar || tFar < 0.0) discard;
    tNear = max(tNear, 0.0); 

    // Jittering
    tNear += random(gl_FragCoord.xy) * u_StepSize;

    vec3 currentPos = localCamPos + rayDir * tNear;
    vec4 accumulatedColor = vec4(0.0);
    
    // Предрасчет множителя коррекции прозрачности
    float alphaCorrection = u_StepSize * 100.0; 

    for(int i = 0; i < MAX_STEPS; i++) {
        // Условие выхода: полная непрозрачность или выход за границы
        if(accumulatedColor.a >= 0.99) break; 
        
        // ВАЖНО: Проверка выхода за пределы текстуры вручную, 
        // так как tFar может быть неточным из-за джиттеринга
        if(max(max(abs(currentPos.x), abs(currentPos.y)), abs(currentPos.z)) > 1.0) break;

        vec3 texCoord = (currentPos + 1.0) * 0.5;
        
        // Читаем всё сразу. Alpha хранит плотность.
        vec4 voxelData = texture(u_VolumeTexture, texCoord);
        float density = voxelData.a;

        if (density > u_DensityThreshold) {
            vec4 srcColor = texture(u_TransferFunction, density);
            
            // --- FIX 1: Защита от NaN и пересвета ---
            // Сначала применяем множитель
            srcColor.a *= u_OpacityMultiplier;
            // Ограничиваем альфу, чтобы она НИКОГДА не была >= 1.0 для коррекции
            srcColor.a = clamp(srcColor.a, 0.0, 0.95); 

            if (srcColor.a > 0.02) {
                // Распаковка нормали из [0,1] в [-1,1]
                vec3 normal = normalize(voxelData.rgb * 2.0 - 1.0);
                
                // --- FIX 2: Упрощенное освещение (Diffuse only) для скорости ---
                float diff = max(dot(normal, u_LightDir), 0.0);
                vec3 lighting = vec3(0.3) + (vec3(0.7) * diff); // Ambient + Diffuse
                srcColor.rgb *= lighting;

                // Коррекция прозрачности
                srcColor.a = 1.0 - pow(1.0 - srcColor.a, alphaCorrection);
                
                // Front-to-back blending
                srcColor.rgb *= srcColor.a;
                accumulatedColor = accumulatedColor + srcColor * (1.0 - accumulatedColor.a);
            }
        }
        
        currentPos += rayDir * u_StepSize;
    }
    
    FragColor = accumulatedColor;
}