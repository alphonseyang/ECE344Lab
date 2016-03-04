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
	struct sorted_points *temp = sp;
	if(sp == NULL){
		return;
	}
	if(sp->p == NULL){
		free(sp);
		sp = NULL;
		return;
	}
	while(temp != NULL){
		struct sorted_points *to_delete = temp;
		temp = temp->next;
		free(to_delete->p);
		to_delete->p = NULL;
		free(to_delete);
		to_delete = NULL;
	}
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
	if(sp == NULL){
		sp = new_sp;
	}else if(sp->p == NULL){
		sp->p = (struct point *)malloc(sizeof(struct point));
		free(new_sp->p);
		new_sp->p = NULL;
		free(new_sp);
		new_sp = NULL;
		sp->p->x = x;
		sp->p->y = y;
		sp->distance = distance;
	}
	else{
		struct sorted_points *temp, *prev;
		temp = sp;  
		prev = sp;
		if(temp->distance > distance || (distance == temp->distance && x < temp->p->x)){    //insert at the first		
			new_sp->p->x = temp->p->x;
			new_sp->p->y = temp->p->y;
			new_sp->distance = temp->distance;
			new_sp->next = temp->next;    //insert one after the first one
			temp->next = new_sp;
			temp->p->x = x;
			temp->p->y = y;
			temp->distance = distance;		
			return 1;
		}
		int i = 0;
		while(temp != NULL && temp->distance <= distance ){
			i++;
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
	if(sp == NULL || sp->p == NULL)
		return 0;
	struct sorted_points *temp, *prev;
	temp = sp;
	ret->x = sp->p->x;
	ret->y = sp->p->y;
	if(sp->next == NULL){
		free(sp->p);
		sp->p = NULL;
		return 1;
	}
	while((temp->next) != NULL){
		temp->p->x = temp->next->p->x;
		temp->p->y = temp->next->p->y;
		temp->distance = temp->next->distance;
		prev = temp;
		temp = temp->next;
	}
	free(temp->p);
	temp->p = NULL;
	free(temp);
	temp = NULL;
	prev->next = NULL;
	return 1;
}

int
sp_remove_last(struct sorted_points *sp, struct point *ret)
{
	if(sp == NULL || sp->p == NULL)
		return 0;
	struct sorted_points *temp, *prev;
	temp = sp;
	prev = sp;
	if(sp->next == NULL){
		ret->x = temp->p->x;
		ret->y = temp->p->y;
		free(temp->p);
		temp->p = NULL;
		return 1;
	}
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
	prev->next = NULL;
	return 1;
}

int
sp_remove_by_index(struct sorted_points *sp, int index, struct point *ret)
{
	if(sp == NULL || sp->p == NULL){
		return 0;
	}
	if(index == 0){
		sp_remove_first(sp, ret);
		return 1;
	}
	int i = 0;
	struct sorted_points *temp, *prev;
	temp = sp;
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
	if(sp == NULL || sp->p == NULL)
		return 0;
	if(sp->next == NULL)
		return 0;
	int record_count = 0;
	struct sorted_points *temp, *prev;
	temp = sp;
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
			temp = prev;
		}
	}
	return record_count;
}
