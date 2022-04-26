#include <math.h>

#include "canvas/canvas.h"
#include "linal/vec.h"

#include "array.h"
#include "camera.h"
#include "scene.h"
#include "sphere.h"

ViewPort viewport_new(int w, int h, int d) {
    return (ViewPort){ w, h, d };
}

Vec3 viewport_coords(ViewPort *self, int x, int y, Canvas *canvas) {
    return vec3_new((float)x * (float)self->w / canvas->image.w,
                    (float)y * (float)self->h / canvas->image.h,
                    (float)self->d);
}

Scene scene_new(Camera cam, ViewPort viewport, Color bg) {
    SphereArray objects;
    LightArray lights;

    array_init(Sphere, &objects, 8);
    array_init(Light, &lights, 8);

    return (Scene){ bg, cam, viewport, objects, lights };
}

void scene_add_object(Scene *scene, Sphere sphere) {
    array_add(&scene->objects, sphere);
}

void scene_add_light(Scene *scene, Light light) {
    array_add(&scene->lights, light);
}

static float compute_lighting(Scene *scene, Vec3 P, Vec3 N, Vec3 V, float s) {
    float intensity = 0;

    for (int i = 0; i < scene->lights.num; i++) {
        Light *light = &scene->lights.values[i];

        if (light->type == Ambient) {
            intensity += light->intensity;
            continue;
        }

        Vec3 L = (light->type == Point) ? vec3_sub(light->position, P)
                                        : light->direction;
        float NL = vec3_dot(N, L);
        intensity +=
            (NL < 0) ? 0 : light->intensity * NL / (vec3_len(L) * vec3_len(N));

        if (s != -1) {
            Vec3 R = vec3_sub(vec3_mul(N, 2 * NL), L);
            float RV = vec3_dot(R, V);

            if (RV > 0)
                intensity += light->intensity *
                             powf(RV / (vec3_len(R) * vec3_len(V)), s);
        }
    }

    return intensity;
}

static Color trace_ray(Scene *scene, Vec3 d) {
    float closest_t = INFINITY;
    Sphere *closest = NULL;
    float result[2] = { 0 };

    for (int i = 0; i < scene->objects.num; i++) {
        Sphere *object = &scene->objects.values[i];
        if (sphere_intersect_ray(object, &scene->cam, d, result) == 0) {
            for (int n = 0; n < 2; n++) {
                if (result[n] > 1.0 && result[n] < INFINITY &&
                    result[n] < closest_t) {
                    closest_t = result[n];
                    closest = object;
                }
            }
        }
    }

    if (closest == NULL)
        return scene->bg;

    Vec3 P = vec3_add(scene->cam.pos, vec3_mul(d, closest_t));
    Vec3 N = vec3_norm(vec3_sub(P, closest->pos));

    return color_mul(
        closest->color,
        compute_lighting(scene, P, N, vec3_mul(d, -1), closest->specular));
}

void scene_render(Scene *scene, Canvas *canvas) {
    for (int x = -canvas->hw; x < canvas->hw; x++) {
        for (int y = canvas->hh; y > -canvas->hh; y--)
            canvas_set_pixel(canvas, x, y,
                             trace_ray(scene, viewport_coords(&scene->viewport,
                                                              x, y, canvas)));
    }
}
