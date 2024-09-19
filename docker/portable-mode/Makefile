.PHONY: all save

T := portable-mode
S := ../..
D := docker/$(T)
TAG ?= $(T)
OPT ?=

all:
	tar cf repo.tar Dockerfile && \
	(cd $(S) && git ls-files --recurse-submodules -z | xargs -0 tar rf $(D)/repo.tar) && \
	rm -f repo.tar.gz && gzip -1 repo.tar && \
	docker buildx build --tag charm-sycl:$(TAG) $(OPT) - < repo.tar.gz; rm -f repo.tar repo.tar.gz

save:
	id=$$(docker create --rm charm-sycl:$(TAG) tail -F /dev/null) && \
	trap "docker rm -f $$id >/dev/null" EXIT && \
	docker start $${id} >/dev/null && \
	docker exec -w /usr/local/bin $${id} sha512sum cscc && \
	docker exec -w /usr/local/bin $${id} gzip -1 cscc && \
	docker cp $${id}:/usr/local/bin/cscc.gz . && gunzip cscc.gz 
