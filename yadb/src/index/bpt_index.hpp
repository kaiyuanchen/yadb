#pragma once

#include <algorithm>
#include <cstring>
#include <iostream>
#include <memory>
#include <map>
#include <queue>
#include <stdint.h>
#include <stdio.h>
#include <stddef.h>
#include <boost/algorithm/string/join.hpp>
#include <boost/filesystem.hpp>
#include "index/bpt_fs.hpp"
#include "index/bpt_nodebase.hpp"
#include "index/bpt_node.hpp"

#include "yadb.hpp"
using namespace std;
namespace boost_fs = boost::filesystem;

KV class leaf_node;
KV class index_node;

#define BPT_CONSTRUCT KV\
	bpt_index<K, V>::bpt_index
#define BPT_DESTRUCT KV\
	bpt_index<K, V>::~bpt_index
#define BPT_START KV\
	RC bpt_index<K, V>::start
#define BPT_CREATE_LEAF KV\
	shared_ptr<leaf_node<K, V>> bpt_index<K, V>::create_leaf_node
#define BPT_CREATE_INDEX KV\
	shared_ptr<index_node<K, V>> bpt_index<K, V>::create_index_node
#define BPT_GET_LEAF KV\
	shared_ptr<leaf_node<K, V>> bpt_index<K, V>::get_leaf_node
#define BPT_GET_INDEX KV\
	shared_ptr<index_node<K, V>> bpt_index<K, V>::get_index_node
#define BPT_INSERT KV\
	RC bpt_index<K, V>::insert
#define BPT_SEARCH KV\
	RC bpt_index<K, V>::search
#define BPT_STOP KV\
	RC bpt_index<K, V>::stop
#define BPT_COMMIT KV\
	RC bpt_index<K, V>::commit
#define BPT_PRINT KV\
	RC bpt_index<K, V>::print

template<class K, class V>
class bpt_index : public enable_shared_from_this<bpt_index<K, V>>{
	friend class leaf_node<K, V>;
	friend class index_node< K, V>;
public:
	bpt_index(){
		fs = new bpt_fs();
	};
	~bpt_index(){
		delete fs;
	};
	RC start(const char* path = nullptr){
		
		if (path && boost_fs::exists(path)){
			fs->open(path);
			fs->read_header(&meta, sizeof(meta));
			for (int i = 0; i < meta.leaf_allocated; i += 2){
				auto leaf = make_shared<leaf_node<K, V>>(i, this->shared_from_this());
				fs->read(i, &leaf->node, sizeof(leaf->node));
				nodes.insert(make_pair(i, leaf));
			}
			for (int i = 1; i < meta.index_allocated; i += 2){
				bool is_dummy = (i == 1) ? true : false;
				auto index = make_shared<index_node<K, V>>(i, is_dummy, this->shared_from_this());
				fs->read(i, &index->node, sizeof(index->node));
				nodes.insert(make_pair(i, index));

				//set dummy
				if (is_dummy) dummy = index;
			}
		}
		else if (path){
			fs->open(path);
			meta.index_allocated = 1;
			meta.leaf_allocated = 0;
			dummy = create_index_node(true);
			//DEBUG("DUMMY1@%d", dummy->get_id());
			auto root = create_leaf_node();
			dummy->set_dummy(root->get_id(), root->type);
		}
		else{
			meta.index_allocated = 1;
			meta.leaf_allocated = 0;
			dummy = create_index_node(true);
			//DEBUG("DUMMY2@%d", dummy->get_id());
			auto root = create_leaf_node();
			dummy->set_dummy(root->get_id(), root->type);
		}
	};
	shared_ptr<leaf_node<K, V>> create_leaf_node(){
		auto node = make_shared<leaf_node<K, V>>(meta.leaf_allocated, this->shared_from_this());
		nodes.insert(make_pair(meta.leaf_allocated, node));
		meta.leaf_allocated += 2;
		return node;
	};
	shared_ptr<index_node<K, V>> create_index_node(bool is_dummy = false){
		auto node = make_shared<index_node<K, V>>(meta.index_allocated, is_dummy, this->shared_from_this());
		nodes.insert(make_pair(meta.index_allocated, node));
		meta.index_allocated += 2;
		return node;
	};
	shared_ptr<leaf_node<K, V>> get_leaf_node(int node_id){
		auto it = nodes.find(node_id);
		if (it != nodes.end()) return static_pointer_cast<leaf_node<K, V>>(it->second);
		throw;
	};
	shared_ptr<index_node<K, V>> get_index_node(int node_id){
		auto it = nodes.find(node_id);
		if (it != nodes.end()) return static_pointer_cast<index_node<K, V>>(it->second);
	};
	RC insert(K key, V val){
		shared_ptr<index_node<K, V>> foo = nullptr;
		dummy->insert(key, val, foo);
	};
	RC search(K key, V* ret){
		return dummy->select(key, ret);
	};
	RC search(K key1, K key2, vector<V>* ret){
		bpt_cursor<K> cursor;
		cursor.key1 = key1;
		dummy->locate(&cursor);
		auto ptr_node = get_leaf_node(cursor.leaf_id);
		auto ptr_rec = ptr_node->node.rec + cursor.rec_off;

		while (true){
			ret->push_back(ptr_rec->value);
			ptr_rec++;
			
			if (ptr_node->comapre_rec(ptr_rec, key2)==RC::ERROR) break;
			if (ptr_node->end() == ptr_rec && ptr_node->node.next == 0) break;
			else if (ptr_node->end() == ptr_rec){
				ptr_node = get_leaf_node(ptr_node->node.next);
				ptr_rec = ptr_node->node.rec;
			}
		}
	};
	RC remove(K key){};
	RC commit(){
		for (auto rec : nodes){
			if (rec.second->type == bpt_node::NODE_TYPE::LEAF){
				auto leaf = static_pointer_cast<leaf_node<K, V>>(rec.second);
				fs->write(leaf->get_id(), &leaf->node, sizeof(leaf->node));
			}
			else{
				auto index = static_pointer_cast<index_node<K, V>>(rec.second);
				fs->write(index->get_id(), &index->node, sizeof(index->node));
			}
		}
		fs->write_header(&meta, sizeof(meta));
	};
	RC print(){};
	RC stop(){};

	/*
	template< class T = K, typename std::enable_if<std::is_same<T, int>::value, int>::type = 0>
	RC test(K key){
		cout << "int:" << key << endl;
	};

	template< class T = K, typename std::enable_if<std::is_same<T, string>::value, int>::type = 0>
	RC test(K key){
		cout << "str:" << key << endl;
	};

	template< class T = K, typename std::enable_if<std::is_same<T, char[]>::value, int>::type = 0>
	RC test(K key){
		cout << sizeof(key) << endl;
		cout << sizeof(key) << endl;
		cout << "char*:" << key << endl;
	};
	*/
private:
	bpt_fs* fs;
	bpt_header meta;
	shared_ptr<index_node<K, V>> dummy;
	map<int, shared_ptr<bpt_node>> nodes;
};
