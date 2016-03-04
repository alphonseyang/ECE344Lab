#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "common.h"
#include "point.h"
#include "sorted_points.h"

struct sorted_points {
	/* you can define this struct to have whatever fields you want. */
	struct sorted_points *next;   //next pointer
	struct point *p;      //point p value
	double distance;
};

struct sorted_points *sp_init()
{
	struct sorted_points *sp;
	sp = (struct sorted_points *)malloc(sizeof(struct sorted_points));
	assert(sp);
	sp->next = NULL;
	sp->p = NULL;
	return sp;
}

void
sp_destroy(struct sorted_points *sp)
{
	if(sp->next == NULL){
		free(sp);
		sp = NULL;
		return;
	}
	struct sorted_points *temp = sp->next;
	struct sorted_points *next;
	while(temp != NULL){
		next = temp->next;
		free(temp->p);
		temp->p = NULL;
		free(temp);
		temp = NULL;
		temp = next;
	}
	free(sp);
	sp = NULL;
	return;
}

int
sp_add_point(struct sorted_points *sp, double x, double y)
{
	struct sorted_points *new_sp = sp_init();
	assert(new_sp);
	new_sp->p = (struct point *)malloc(sizeof(struct point));
	new_sp->p->x = x;
	new_sp->p->y = y;
	double distance = sqrt(x*x+y*y);
	new_sp->distance = distance;
	if(sp->next == NULL){
		sp->next = new_sp;
	}
	else{
		struct sorted_points *temp, *prev;
		temp = sp->next;  
		prev = sp;
		if(temp->distance > distance || (distance == temp->distance && x < temp->p->x)){    //insert at the first		
			new_sp->next = temp;
			sp->next = new_sp;
			return 1;
		}
		while(temp != NULL && temp->distance <= distance ){
			if(temp->distance == distance){
				if(x < temp->p->x)
					break;
				else if(x == temp->p->x && y < temp->p->y)
					break;
			}
			prev = temp;
			temp = temp->next;
		}	
		new_sp->next = temp;
		prev->next = new_sp;
	}
	return 1;
}

int
sp_remove_first(struct sorted_points *sp, struct point *ret)
{
	if(sp->next == NULL)
		return 0;
	struct sorted_points *temp;
	temp = sp->next;
	ret->x = temp->p->x;
	ret->y = temp->p->y;
	sp->next = temp->next;
	free(temp->p);
	temp->p = NULL;
	free(temp);
	temp = NULL;
	return 1;
}

int
sp_remove_last(struct sorted_points *sp, struct point *ret)
{
	if(sp->next == NULL)
		return 0;
	struct sorted_points *temp, *prev;
	temp = sp->next;
	prev = sp;
	while(temp->next != NULL){
		prev = temp;
		temp = temp->next;
	}
	ret->x = temp->p->x;
	ret->y = temp->p->y;
	free(temp->p);
	temp->p = NULL;
	free(temp);
	temp = NULL;
	if(prev) prev->next = NULL;
	return 1;
}

int
sp_remove_by_index(struct sorted_points *sp, int index, struct point *ret)
{
	if(sp->next == NULL){
		return 0;
	}
	if(index == 0){
		sp_remove_first(sp, ret);
		return 1;
	}
	int i = 0;
	struct sorted_points *temp, *prev;
	temp = sp->next;
	prev = sp;
	for(;i < index; i++){
		if(temp->next == NULL && i != index){
			return 0;
		}
		prev = temp;
		temp = temp->next;
	}
	prev->next = temp->next;
	ret->x = temp->p->x;
	ret->y = temp->p->y;
	temp->next = NULL;
	free(temp->p);
	temp->p = NULL;
	free(temp);
	temp = NULL;
	return 1;
}

int
sp_delete_duplicates(struct sorted_points *sp)
{
	if(sp->next == NULL)
		return 0;
	int record_count = 0;
	struct sorted_points *temp, *prev;
	temp = sp->next;
	prev = sp;
	while(temp != NULL){
		prev = temp;
		temp = temp->next;	
		
		if(temp == NULL){
			break;
		}
		if(prev->p->x == temp->p->x && prev->p->y == temp->p->y){    //find the duplicate record
			record_count++;
			prev->next = temp->next;
			free(temp->p);
			temp->p = NULL;
			free(temp);
			temp = NULL;
			temp = prev;
		}
	}
	return record_count;
}
