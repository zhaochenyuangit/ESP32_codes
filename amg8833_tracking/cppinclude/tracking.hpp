#include "human_object.hpp"
extern "C"
{
#include "feature_extraction.h"
}

struct ObjectNode
{
    HumanObject *ob;
    ObjectNode *next;
};

class ObjectList
{
private:
    ObjectNode *head;
    ObjectNode *tail;
    ObjectNode *p;
    unsigned int n_ob;
    int count;

private:
    int match_centroid(HumanObject *ob, Blob *blob_list, int n_blobs);
    int match_centrals(HumanObject *ob, Blob *blob_list, int n_blobs);
    bool append_object(HumanObject *ob);
    bool delete_object_by_label(int label);
    bool count_and_delete_every_objects();
    void count_an_object_before_delete(ObjectNode *p);

public:
    ObjectList();
    ~ObjectList();

    int get_n_objects();
    ObjectNode *get_head_node();
    int get_count();

    void matching(Blob *blob_list, int n_blobs);
};
