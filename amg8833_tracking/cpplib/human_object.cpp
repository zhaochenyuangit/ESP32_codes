#include "human_object.hpp"

int HumanObject::serial_num_ = 0;

HumanObject::HumanObject(int label, int pos_x, int pos_y, int size)
{
    serial_num_ += 1;
    label_ = serial_num_;
    printf("create object %d from Blob %d at %d %d\n", label_, label, pos_x, pos_y);
    pos_x_ = pos_x;
    pos_y_ = pos_y;
    first_x_ = pos_x;
    first_y_ = pos_y;
    size_ = size;
    last_size_ = size;
    vel_x_ = 0;
    vel_y_ = 0;
}

void HumanObject::ab_filter(int pos_x, int pos_y)
{
    float alpha = 0.75;
    float beta = 0.8;
    int res_x = pos_x - pos_x_;
    int res_y = pos_y - pos_y_;
    pos_x_ += res_x * alpha;
    pos_y_ += res_y * alpha;
    vel_x_ = (1.2 - beta) * vel_x_ + beta * res_x;
    vel_y_ = (1.2 - beta) * vel_y_ + beta * res_y;
}

void HumanObject::update(int pos_x, int pos_y, int size)
{
    printf("before pos: %d, %d vel: %d,%d\n",pos_x_,pos_y_,vel_x_,vel_y_);
    ab_filter(pos_x, pos_y);
    last_size_ = size_;
    size_ = size;
    printf("after pos %d, %d vel %d, %d\n",pos_x_,pos_y_,vel_x_,vel_y_);
}

void HumanObject::predict(int *ppos_x, int *ppos_y)
{
    *ppos_x = pos_x_ + vel_x_;
    *ppos_y = pos_y_ + vel_y_;
}

HumanObject::~HumanObject()
{
    printf("delete object %d\n", label_);
}

void HumanObject::get_shift(int *first_x, int *first_y, int *now_x, int *now_y)
{
    *first_x = first_x_;
    *first_y = first_y_;
    *now_x = pos_x_;
    *now_y = pos_y_;
}

int HumanObject::get_index(){
    return label_;
}