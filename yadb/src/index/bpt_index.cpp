//#include "bpt_index.hpp"
/*
BPT_CONSTRUCT(){
	fs = new bpt_fs();
}

BPT_DESTRUCT(){
	delete fs;
}

BPT_START(const char* path){
	if (path && boost_fs::exists(path)){
		fs->open(path);
		fs->read_header(&meta, sizeof(meta));

		for (int i = 0; i < meta.leaf_allocated; i+=2){
			auto leaf = make_shared<leaf_node<K, V>>(i, this->shared_from_this());
			fs->read(i, &leaf->node, sizeof(leaf->node));
			nodes.insert(make_pair(i, leaf));
		}
		for (int i = 1; i < meta.index_allocated; i+=2){
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
		DEBUG("DUMMY1@%d", dummy->get_id());
		auto root = create_leaf_node();
		dummy->set_dummy(root->get_id(), root->type);
	}
	else{
		meta.index_allocated = 1;
		meta.leaf_allocated = 0;
		dummy = create_index_node(true);
		DEBUG("DUMMY2@%d", dummy->get_id());
		auto root = create_leaf_node();
		dummy->set_dummy(root->get_id(), root->type);
	}
}

BPT_CREATE_LEAF(){
	auto node = make_shared<leaf_node<K, V>>(meta.leaf_allocated, this->shared_from_this());
	nodes.insert(make_pair(meta.leaf_allocated, node));
	meta.leaf_allocated += 2;
	return node;
}

BPT_CREATE_INDEX(bool is_dummy){
	auto node = make_shared<index_node<K, V>>(meta.index_allocated, is_dummy, this->shared_from_this());
	nodes.insert(make_pair(meta.index_allocated, node));
	meta.index_allocated += 2;
	return node;
}

BPT_GET_LEAF(int node_id){
	auto it = nodes.find(node_id);
	if (it != nodes.end()) return static_pointer_cast<leaf_node<K, V>>(it->second);
	throw;
}

BPT_GET_INDEX(int node_id){
	auto it = nodes.find(node_id);
	if (it != nodes.end()) return static_pointer_cast<index_node<K, V>>(it->second);
}

BPT_INSERT(K key, V val){
	shared_ptr<index_node<K, V>> foo = nullptr;
	dummy->insert(key, val, foo);
}

BPT_SEARCH(K key, V* ret){
	return dummy->select(key, ret);
}

BPT_STOP(){
	dummy = nullptr;
	nodes.clear();
}

BPT_COMMIT(){
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
}

//template<class K, class V>
//typename std::enable_if<std::is_same<K, int>::value, bool>::type
//bpt_index<K, V>::test(K key){}

BPT_PRINT(){
	queue<pair<shared_ptr<index_node<K, V>>, int>> indexs;
	vector<shared_ptr<leaf_node<K, V>>> leafs;

	if (dummy->node.child_type == bpt_node::NODE_TYPE::LEAF) leafs.push_back(get_leaf_node(dummy->node.rec[0].value));
	else indexs.push(make_pair(get_index_node(dummy->node.rec[0].value), 1));

	int level = 1;
	while (indexs.size() > 0){
		auto p = indexs.front();
		indexs.pop();

		auto node = p.first;
		int h = p.second;

		if (h != level) {
			level = h;
			cout << endl;
		}

		vector<string> keys;
		for (int i = 0; i < node->order; i++) keys.push_back(to_string(node->node.rec[i].key));
		//if (!is_integral<K>::value) for (int i = 0; i < node->order; i++) keys.push_back(to_string(node->node.rec[i].key));
		//else for (int i = 0; i < node->order; i++) keys.push_back(node->node.rec[i].key);

		string str = boost::algorithm::join(keys, ",");
		std::cout << node->get_id() << "," << node->node.key_size << "(" + str + ") ";

		for (int i = 0; i < node->node.key_size; i++){
			int child_id = node->node.rec[i].value;
			if (node->node.child_type == bpt_node::NODE_TYPE::INDEX) indexs.push(make_pair(get_index_node(child_id), h + 1));
			else if (node->node.child_type == bpt_node::NODE_TYPE::LEAF) leafs.push_back(get_leaf_node(child_id));
		}
	}

	cout << endl;
	for_each(leafs.begin(), leafs.end(), [](shared_ptr<leaf_node<K, V>> leaf){
		vector<string> keys;
		for (int i = 0; i < leaf->order; i++) keys.push_back(to_string(leaf->node.rec[i].key));
		string str = boost::algorithm::join(keys, ",");
		std::cout << leaf->get_id() << "," << leaf->node.key_size << "(" + str + ") ";
	});
	cout << endl;
}
*/
