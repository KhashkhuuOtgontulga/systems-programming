#include <stdlib.h>
#include <string.h>
#include "hashtable.h"

/* Daniel J. Bernstein's "times 33" string hash function, from comp.lang.C;
   See https://groups.google.com/forum/#!topic/comp.lang.c/lSKWXiuNOAk */
unsigned long hash(char *str) {
  unsigned long hash = 5381;
  int c;

  while ((c = *str++))
    hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

  return hash;
}

hashtable_t *make_hashtable(unsigned long size) {
  hashtable_t *ht = malloc(sizeof(hashtable_t));
  ht->size = size;
  ht->buckets = calloc(sizeof(bucket_t *), size);
  return ht;
}

void ht_put(hashtable_t *ht, char *key, void *val) {
  /* FIXME: the current implementation doesn't update existing entries */
  unsigned int idx = hash(key) % ht->size;
  bucket_t *b = ht->buckets[idx];
  while (b) { // search the entire bucket
	if (strcmp(b->key,key)==0) { 
		free(b->val);
		free(key);
		b->val = val;
		return;
	}
	b = b->next;
  }
  // if key not found then add the new bucket
  b = malloc(sizeof(bucket_t));
  b->key = key;
  b->val = val;
  b->next = ht->buckets[idx];
  ht->buckets[idx] = b;
}

void *ht_get(hashtable_t *ht, char *key) {
  unsigned int idx = hash(key) % ht->size;
  bucket_t *b = ht->buckets[idx];
  while (b) {
    if (strcmp(b->key, key) == 0) {
      return b->val;
    }
    b = b->next;
  }
  return NULL;
}

void ht_iter(hashtable_t *ht, int (*f)(char *, void *)) {
  bucket_t *b;
  unsigned long i;
  for (i=0; i<ht->size; i++) {
    b = ht->buckets[i];
    while (b) {
      if (!f(b->key, b->val)) {
        return ; // abort iteration
      }
      b = b->next;
    }
  }
}

// free bucket
void free_bucket(bucket_t *b) {
	free(b->key);
	free(b->val);
	free(b);
}

void free_hashtable(hashtable_t *ht) {
  // FIXME: must free all substructures!
  bucket_t *b, *temp;
  unsigned long i;
  for (i=0; i<ht->size; i++) {
	b = ht->buckets[i];
	while (b) {
	   temp = b->next;
	   free_bucket(b);
	   b = temp;
	}
  }
  free(ht->buckets); // free the underlying buckets of the calloc call  
  free(ht); // free hashtable 
}


/* TODO */
void  ht_del(hashtable_t *ht, char *key) {
	unsigned int idx = hash(key) % ht->size;
	bucket_t *b = ht->buckets[idx]; 
	bucket_t *prev;
	// if the first bucket
	if (strcmp(b->key,key)==0) {
		ht->buckets[idx] = b->next;
		free_bucket(b);
		return;
	}
	// else the bucket is somewhere else
	else {
		prev = b;
		b = b->next;
		while (b) {
			if (strcmp(b->key,key)==0) {
				prev->next = b->next;
				free_bucket(b);
				return;
			}
			prev = prev->next;
			b = b->next;
		}
	}
}
void  ht_rehash(hashtable_t *ht, unsigned long newsize) {
  hashtable_t *nht = make_hashtable(newsize);
  unsigned long i;
  bucket_t *b, *temp;
  for (i=0;i<ht->size;i++) {
	b = ht->buckets[i];
	while (b) {	
		temp = b->next;
		ht_put(nht,b->key,b->val);
		free(b);
		b = temp;
	}
  }
  free(ht->buckets);
  ht->size = nht->size;
  *ht = *nht;
  free(nht);
}
