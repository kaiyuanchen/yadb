#include "fde.hpp"

fd_events::fd_events(int timeout_){
	efd = epoll_create1(0);
	events = static_cast<epoll_event*>(calloc(MAX_FD, sizeof(epoll_event)));
	timeout = timeout_* 1000;
}

fd_events::~fd_events(){
	free(events);
}

int fd_events::wait(){
	return epoll_wait(efd, events, MAX_FD, timeout ? timeout : -1);
}

RC fd_events::remove(shared_ptr<connection> conn){
	int fd = conn->get_fd();
	conn->close();
	epoll_event curr_event = events[fd];
	delete static_cast<fd_event*>(curr_event.data.ptr);
	curr_event.data.ptr = nullptr;
	return RC::OK;
}

shared_ptr<connection> fd_events::get_conn(int fd){
	epoll_event curr_event = events[fd];
	struct fd_event *fde = static_cast<fd_event*>(curr_event.data.ptr);
	assert(fde);

	if ((curr_event.events & EPOLLERR) ||
		(curr_event.events & EPOLLHUP) ||	//error
		(curr_event.events & EPOLLRDHUP)) { //close
		remove(fde->conn);
		return nullptr;
	}
	else return fde->conn;

};

RC fd_events::set(shared_ptr<connection> conn){
	int ret;
	int fd = conn->get_fd();

	struct fd_event *fde = new fd_event();
	fde->conn = conn;

	epoll_event event;
	event.data.fd = fd;
	event.data.ptr = static_cast<void*>(fde);
	event.events = EPOLLIN | EPOLLRDHUP;
	ret = epoll_ctl(efd, EPOLL_CTL_ADD, fd, &event);
	events[fd] = event;
	return RC::OK;
}

