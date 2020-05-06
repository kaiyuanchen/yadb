#pragma once

#include <algorithm>
#include <cstring>
#include <iostream>
#include <memory>
#include <stdint.h>
#include <stdio.h>
#include <stddef.h>
#include "index/bpt_fs.hpp"
#include "index/bpt_index.hpp"
#include "index/bpt_nodebase.hpp"
using namespace std;
KV class bpt_index;
KV class index_node;
KV class leaf_node;
template<class K> class bpt_cursor;

#define LEAF_ORDER(rec)	(BPT_BLOCK_SIZE- sizeof(uint32_t)* 4) / sizeof(rec)
#define INDEX_ORDER(rec) (BPT_BLOCK_SIZE- sizeof(uint32_t)* 3) / sizeof(rec)
#define COMP(type) \
	struct comp{\
		bool operator()(const type& node, const int& i) const { return node.key < i; }\
		bool operator()(const int& i, const type& node) { return node.key > i; }\
		bool operator()(char* str, const type& node) { return strcmp(str, node.key); }\
		bool operator()(const type& node, char* str) { return strcmp(node.key, str); }\
	};
#define COPY(tar, src)\
	if (is_array<K>::value || is_pointer<K>::value) memcpy(&tar, (void*)(intptr_t)src, sizeof(K));\
	else memcpy(&tar, &src, sizeof(K));

#define LEAF_CONSTRUCT KV\
	leaf_node<K, V>::leaf_node
#define LEAF_SAVE KV\
	RC leaf_node<K, V>::save
#define LEAF_READ KV\
	RC leaf_node<K, V>::read
#define LEAF_INSERT KV\
	RC leaf_node<K, V>::insert
#define LEAF_INSERT_RECORD KV\
	RC leaf_node<K, V>::insert_record
#define LEAF_SELECT KV\
	RC leaf_node<K, V>::select

#define INDEX_CONSTRUCT KV\
	index_node<K, V>::index_node
#define INDEX_SAVE KV\
	RC index_node<K, V>::save
#define INDEX_READ KV\
	RC index_node<K, V>::read
#define INDEX_INSERT KV\
	RC index_node<K, V>::insert
#define INDEX_INSERT_AND_SPLIT KV\
	RC index_node<K, V>::insert_and_split
#define INDEX_INSERT_RECORD KV\
	RC index_node<K, V>::insert_record
#define INDEX_SELECT KV\
	RC index_node<K, V>::select
#define INDEX_SET_DUMMY KV\
	RC index_node<K, V>::set_dummy
#define INDEX_SET_ROOT KV\
	RC index_node<K, V>::set_root

template<class K, class V>
class index_node : public bpt_node{
public:
	typedef struct{
		K key;
		int32_t value = 0;
	} index_record_t;
	typedef struct{
		int32_t key_size = 0;
		NODE_TYPE type = NODE_TYPE::INDEX;
		NODE_TYPE child_type = NODE_TYPE::LEAF;
		index_record_t rec[INDEX_ORDER(index_record_t)];  //max key== INDEX_ORDER- 1
	} index_node_t;
	COMP(index_record_t);

	explicit index_node(int node_id_, bool is_dummy, shared_ptr<bpt_index<K, V>> bpt_){
		if (std::is_pod<K>::value == false) throw;
		memset(&this->node, 0, sizeof(index_node_t));
		node_id = node_id_;
		bpt = bpt_;
		type = is_dummy ? NODE_TYPE::DUMMY : NODE_TYPE::INDEX;
	};
	RC set_dummy(int32_t node_id, NODE_TYPE node_type){
		if (type != NODE_TYPE::DUMMY) throw;
		node.rec[0].value = node_id;
		node.child_type = node_type;
	};
	RC set_root(int32_t left_node_id, int32_t right_node_id, K key){
		COPY(node.rec[0].key, key)
		node.rec[0].value = left_node_id;
		node.rec[1].value = right_node_id;
		node.key_size = 2;
	};
	RC insert(K key, V val, shared_ptr<index_node>& sibling){
		index_record_t* r = (type == NODE_TYPE::DUMMY) ? node.rec : upper_bound(begin(node), end(node) - 1, key, comp());
		shared_ptr<bpt_node> new_child = nullptr;
		K new_key;

		if (node.child_type == NODE_TYPE::LEAF){
			shared_ptr<leaf_node<K, V>> new_leaf = nullptr;
			auto child = bpt->get_leaf_node(r->value);
			child->insert(key, val, new_leaf);
			new_child = new_leaf;
			//if (new_child) new_key = new_leaf->node.rec[0].key;
			if (new_child) { COPY(new_key, new_leaf->node.rec[0].key) }

		}
		else{
			shared_ptr<index_node<K, V>> new_index = nullptr;
			auto child = bpt->get_index_node(r->value);
			child->insert(key, val, new_index);
			new_child = new_index;
			//if (new_index) new_key = new_index->middle_key;
			if (new_child) { COPY(new_key, new_index->middle_key) }
		}

		if (new_child && type == NODE_TYPE::DUMMY){

			auto new_root = bpt->create_index_node();
			new_root->set_root(r->value, new_child->get_id(), new_key);
			//DEBUG("set root %d %d", r->value, new_child->get_id());
			new_root->node.child_type = new_child->type;
			set_dummy(new_root->get_id(), new_root->type);
			//DEBUG("set dummy %d", new_root->get_id());
		}
		else if (new_child && node.key_size == order){

			sibling = bpt->create_index_node();
			insert_and_split(new_child, sibling);
		}
		else if (new_child){
			insert_record(new_key, new_child->get_id());
		}
	};
	RC select(K key, V* ret){
		auto pos = upper_bound(begin(node), end(node) - 1, key, comp());
		if (node.child_type == NODE_TYPE::LEAF){
			auto child = bpt->get_leaf_node(pos->value);
			return child->select(key, ret);
		}
		else{
			auto child = bpt->get_index_node(pos->value);
			return child->select(key, ret);
		}
	};
	RC remove(){};
	RC locate(bpt_cursor<K>* cursor){
		auto pos = upper_bound(begin(node), end(node) - 1, cursor->key1, comp());
		if (node.child_type == NODE_TYPE::LEAF){
			auto child = bpt->get_leaf_node(pos->value);
			return child->locate(cursor);
		}
		else{
			auto child = bpt->get_index_node(pos->value);
			return child->locate(cursor);
		}
	}
	index_node_t node;
	K middle_key;
	const int order = INDEX_ORDER(index_record_t);
private:
	
	RC insert_and_split(shared_ptr<bpt_node> child, shared_ptr<index_node> sibling){
		K key;

		if (child->type == NODE_TYPE::LEAF){
			auto leaf = static_pointer_cast<leaf_node<K, V>>(child);
			//key = leaf->node.rec[0].key;
			COPY(key, leaf->node.rec[0].key)
		}
		else{
			auto index = static_pointer_cast<index_node<K, V>>(child);
			//key = index->node.rec[0].key;
			COPY(key, index->node.rec[0].key)
		}

		auto split_pos = (node.key_size - 1) / 2;
		bool place_right = comp()(node.rec[split_pos], key) == true;
		if (place_right) split_pos++;
		if (place_right && comp()(node.rec[split_pos], key) == false) split_pos--;

		memcpy(sibling->node.rec, node.rec + split_pos, sizeof(index_record_t)*(node.key_size - split_pos));
		sibling->node.child_type = child->type;
		sibling->node.key_size = node.key_size - split_pos;
		node.key_size = split_pos;
		COPY(sibling->middle_key, node.rec[split_pos - 1].key)

			if (child->type == NODE_TYPE::LEAF){
				if (place_right) sibling->insert_record(key, child->get_id());
				else insert_record(key, child->get_id());
			}
			else{
				if (place_right){
					auto index = static_pointer_cast<index_node<K, V>>(child);
					//K child_middle_key = index->middle_key;
					K child_middle_key;
					COPY(child_middle_key, index->middle_key)
						sibling->insert_record(child_middle_key, child->get_id());
				}
				else insert_record(key, child->get_id());
			}

			return RC::OK;
	};
	RC insert_record(K key, int32_t child_node_id){
		auto pos = upper_bound(begin(node), end(node) - 1, key, comp());
		std::copy_backward(pos, end(node), end(node) + 1);

		//insert key
		if (is_array<K>::value || is_pointer<K>::value) memcpy(&pos->key, (void*)(intptr_t)key, sizeof(K));
		else memcpy(&pos->key, &key, sizeof(K));

		//shift and insert value
		pos->value = (pos + 1)->value;
		(pos + 1)->value = child_node_id;
		node.key_size++;
	};
	RC borrow_key(){};
	RC merge_key(){};
	RC read(){ return bpt->fs->read(node_id, &node, sizeof(node)); };
	RC save(){ return bpt->fs->write(node_id, &node, sizeof(node)); };
	inline index_record_t* begin(index_node_t& node){ return this->node.rec; }
	inline index_record_t* end(index_node_t& node){ return this->node.rec + node.key_size; }

	shared_ptr<bpt_index<K, V>> bpt = nullptr;
};

template<class K, class V>
class leaf_node : public bpt_node {
	friend bpt_index <K, V> ;
	friend index_node<K, V>;
public:
	typedef struct {
		K key;
		V value;
	} leaf_record_t;
	typedef struct {
		uint32_t key_size = 0;
		NODE_TYPE type = NODE_TYPE::LEAF;
		uint32_t next = 0;
		uint32_t prev = 0;
		leaf_record_t rec[LEAF_ORDER(leaf_record_t)];
	} leaf_node_t;
	COMP(leaf_record_t);

	explicit leaf_node(int node_id_, shared_ptr<bpt_index<K, V>> bpt_){
		memset(&node, 0, sizeof(leaf_node_t));
		node_id = node_id_;
		bpt = bpt_;
		type = NODE_TYPE::LEAF;
	};
	RC insert(K key, V val, shared_ptr<leaf_node<K, V>>& _sibling){
		if (node.key_size == order){ //full
			_sibling = bpt->create_leaf_node();
			auto split_pos = (node.key_size - 1) / 2;

			//det pos
			bool place_right = comp()(node.rec[split_pos], key) == true;
			if (place_right) split_pos++;

			//split
			//std::copy(node.rec + split_pos, node.rec + node.key_size, _sibling->node.rec);
			memcpy(_sibling->node.rec, node.rec + split_pos, sizeof(leaf_record_t)*(node.key_size - split_pos));
			_sibling->node.key_size = node.key_size - split_pos;
			node.key_size = split_pos;

			//link
			_sibling->node.prev = node_id;
			node.next = _sibling->node_id;

			//insert rec
			if (place_right) _sibling->insert_record(key, val);
			else insert_record(key, val);
		}
		else{
			insert_record(key, val);
		}
	};
	RC insert_record(K key, V val){
		bool ret = binary_search(begin(node), end(node), key, comp());
		if (!ret){
			auto pos = lower_bound(begin(node), end(node), key, comp());
			std::copy_backward(pos, end(node), end(node) + 1);

			//copy
			//if (is_array<K>::value || is_pointer<K>::value) memcpy(&pos->key, (void*)(intptr_t)key, sizeof(K));
			memcpy(&pos->key, &key, sizeof(K));
			//if (is_array<V>::value || is_pointer<V>::value) memcpy(&pos->value, (void*)(intptr_t)val, sizeof(V));
			memcpy(&pos->value, &val, sizeof(V));

			node.key_size++;
		}
	};
	RC select(K key, V* ret){
		auto retval = binary_search(begin(node), end(node), key, comp());
		if (retval){
			auto pos = lower_bound(begin(node), end(node), key, comp());
			memcpy(ret, &pos->value, sizeof(V));
			return RC::OK;
		}
		else{
			DEBUG("NOT FOUND\n");
			return RC::ERROR;
		}
	};
	RC remove(){};
	RC locate(bpt_cursor<K>* cursor){
		leaf_record_t* pos = lower_bound(begin(node), end(node), cursor->key1, comp());

		//if (pos != begin(node) && comp()(*(pos - 1), cursor->key1) >= 0) pos--;
		int off = pos - begin(node);
		cursor->leaf_id = node_id;
		cursor->rec_off = off;
	};
	RC comapre_rec(leaf_record_t* rec, K key){
		if (comp()(*rec, key)) return RC::OK;
		else
			if (comp()(key, *rec)) RC::ERROR;
			else return RC::OK;
	}
	leaf_node_t node;
	const int order = LEAF_ORDER(leaf_record_t);
private:
	RC borrow_key();
	RC merge_key();
	RC read(){ return bpt->fs->write(node_id, &node, sizeof(node)); };
	RC save(){ return bpt->fs->read(node_id, &node, sizeof(node)); };

	inline leaf_record_t* begin(leaf_node_t& node){ return this->node.rec; }
	inline leaf_record_t* end(leaf_node_t& node){ return this->node.rec + node.key_size; }
	inline leaf_record_t* begin(){ return this->node.rec; }
	inline leaf_record_t* end(){ return this->node.rec + node.key_size; }
	
	shared_ptr<bpt_index<K, V>> bpt = nullptr;
};


/*
 * cursor
 */
template<class K>
class bpt_cursor {
public:
	K key1;
	K key2;
	int leaf_id = 0;
	int rec_off = 0;
};