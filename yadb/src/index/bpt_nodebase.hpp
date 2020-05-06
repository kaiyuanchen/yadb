#pragma once

class bpt_node{
public:
	enum class NODE_TYPE{
		LEAF,
		INDEX,
		DUMMY
	};
	NODE_TYPE type = NODE_TYPE::LEAF;
	inline int32_t get_id(){ return node_id; };
protected:
	int32_t node_id = 0;
	
	virtual RC read() = 0;
	virtual RC save() = 0;

	template<class T>
	static inline typename T::record_t begin(T& node) {
		return node.rec;
	}
	template<class T>
	static inline typename T::record_t end(T& node) {
		return node.rec + node.key_size;
	}
};