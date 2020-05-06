#pragma once
#include <functional>
#include <map>
#include <mutex>
#include <memory>
#include <vector>
#include "yadb.hpp"
using namespace std;

template <class K, class V>
class threadsafe_map
{
public:
	typedef function<bool(V)> filter_fn;

	threadsafe_map();
	RC insert(K key, V value);
	RC filter(filter_fn fn, vector<V>* ret);
	RC itr(vector<pair<K, V>>* ret);
	int size();
private:
	mutex mutex4hash;
	map<K, V> hash;
};


template <class K, class V>
threadsafe_map<K, V>::threadsafe_map(){
}

template <class K, class V>
RC threadsafe_map<K, V>::insert(K key, V value){
	lock_guard<mutex> lock(mutex4hash);
	hash.insert(make_pair(key, value));
}

template <class K, class V>
RC threadsafe_map<K, V>::itr(vector<pair<K, V>>* ret){
	lock_guard<mutex> lock(mutex4hash);
	for (auto val : hash) ret->push_back(val);
	return RC::OK;
}

template <class K, class V>
int threadsafe_map<K, V>::size(){
	lock_guard<mutex> lock(mutex4hash);
	return hash.size();
}

template <class K, class V>
RC threadsafe_map<K, V>::filter(filter_fn fn, vector<V>* ret){
	for (auto val : hash)
		if (fn(val.second)) ret->push_back(val.second);
	return RC::OK;
}