//#include "bpt_node.hpp"
/*
 * leaf node
 */
/*
LEAF_CONSTRUCT(int node_id_, shared_ptr<bpt_index<K, V>> bpt_){
	//if (std::is_pod<K>::value == false) throw;
	//if (std::is_pod<V>::value == false) throw;

	memset(&node, 0, sizeof(leaf_node_t));
	node_id = node_id_;
	bpt = bpt_;
	type = NODE_TYPE::LEAF;
}

LEAF_INSERT(K key, V val, shared_ptr<leaf_node<K, V>>& _sibling){
	if (node.key_size == order){ //full
		_sibling = bpt->create_leaf_node();
		auto split_pos = (node.key_size- 1) / 2;

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
}

LEAF_INSERT_RECORD(K key, V val){
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
}

LEAF_SELECT(K key, V* ret){
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
}

LEAF_SAVE(){ return bpt->fs->write(node_id, &node, sizeof(node)); }
LEAF_READ(){ return bpt->fs->read(node_id, &node, sizeof(node)); }
*/

/*
 * index node
 */
/*
/*
INDEX_CONSTRUCT(int node_id_, bool is_dummy, shared_ptr<bpt_index<K, V>> bpt_){
	if (std::is_pod<K>::value == false) throw;
	memset(&this->node, 0, sizeof(index_node_t));
	node_id = node_id_;
	bpt = bpt_; 
	type = is_dummy ? NODE_TYPE::DUMMY : NODE_TYPE::INDEX;
}

INDEX_INSERT(K key, V val, shared_ptr<index_node>& sibling){
	index_record_t* r = (type == NODE_TYPE::DUMMY) ? node.rec : upper_bound(begin(node), end(node) -1 , key, comp());
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
}

INDEX_INSERT_AND_SPLIT(shared_ptr<bpt_node> child, shared_ptr<index_node> sibling){
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

	auto split_pos = (node.key_size- 1) / 2;
	bool place_right = comp()(node.rec[split_pos], key) == true;
	if (place_right) split_pos++;
	if (place_right && comp()(node.rec[split_pos], key) == false) split_pos--;

	memcpy(sibling->node.rec, node.rec + split_pos, sizeof(index_record_t)*(node.key_size - split_pos));
	sibling->node.child_type = child->type;
	sibling->node.key_size = node.key_size - split_pos;
	node.key_size = split_pos;
	COPY(sibling->middle_key, node.rec[split_pos-1].key)

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
}

INDEX_INSERT_RECORD(K key, int child_node_id){
	auto pos = upper_bound(begin(node), end(node) -1, key, comp());
	std::copy_backward(pos, end(node), end(node) + 1);

	//insert key
	if (is_array<K>::value || is_pointer<K>::value) memcpy(&pos->key, (void*)(intptr_t)key, sizeof(K));
	else memcpy(&pos->key, &key, sizeof(K));

	//shift and insert value
	pos->value = (pos + 1)->value;
	(pos + 1)->value = child_node_id;
	node.key_size++;
}

INDEX_SET_DUMMY(int32_t node_id, NODE_TYPE node_type){
	if (type != NODE_TYPE::DUMMY) throw;
	node.rec[0].value = node_id;
	node.child_type = node_type;
}

INDEX_SET_ROOT(int32_t left_node_id, int32_t right_node_id, K key){
	//node.rec[0].key = key;
	COPY(node.rec[0].key, key)
	node.rec[0].value = left_node_id;
	node.rec[1].value = right_node_id;
	node.key_size = 2;
}

INDEX_SELECT(K key, V* ret){
	auto pos = upper_bound(begin(node), end(node) - 1, key, comp());
	if (node.child_type == NODE_TYPE::LEAF){
		auto child = bpt->get_leaf_node(pos->value);
		return child->select(key, ret);
	}
	else{
		auto child = bpt->get_index_node(pos->value);
		return child->select(key, ret);
	}
}

INDEX_SAVE(){ return bpt->fs->write(node_id, &node, sizeof(node)); }
INDEX_READ(){ return bpt->fs->read(node_id, &node, sizeof(node)); }
*/