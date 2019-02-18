#include <stdlib.h>
#include <string.h>

#include <jansson.h>

int main(int argc, char *argv[])
{
    size_t i;
    char *text;

    json_t *root;

    json_t *ts = json_real(0.01);
    json_t *width_t  = json_integer(640);
    json_t *height_t = json_integer(480);

    json_t *obj_id = json_integer(1);
    json_t *label_id = json_integer(0);
    json_t *confi = json_real(1.0);
    json_t *version = json_integer(1);

    json_t *x_min = json_real(0.1);
    json_t *x_max = json_real(0.2);
    json_t *y_min = json_real(0.3);
    json_t *y_max = json_real(0.4);

    json_t *det = json_pack("{s:{s:{s:o,s:o,s:o,s:o,},s:o,s:o,s:{s:s,s:o,},}}",
                            "detection","bounding_box",
                            "x_min", x_min,
                            "x_max", x_max,
                            "y_min", y_min,
                            "y_max", y_max,
                            "object_id", obj_id,
                            "label_id", label_id,
                            "model", "name", "abc", "version", version);

    json_t *emotion = json_pack("{s:s,s:o,s:o,s:{s:s,s:o},}",
                                "lable", "happy",
                                "lable_id", label_id,
                                "confidence", confi,
                                "model", "name", "abc", "version", version);
    json_object_set_new(det, "emotions", emotion);

    json_t *obj_array = json_array();

    json_array_append_new(obj_array, det);
    json_array_append_new(obj_array, det);

    root = json_pack("{s:o,s:s,s:{s:o,s:o,},s:o}",
                     "timestamp",ts,
                     "source","url",
                     "resolution","width",width_t,"height",height_t,"objects",obj_array);

    text = json_dumps(root, JSON_INDENT(4));
    puts(text);
    free(text);

    json_decref(obj_array);
    json_decref(root);

    return(0);
}
