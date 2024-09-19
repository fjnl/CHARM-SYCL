.PHONY: all

T := ubuntu22.04-llvm13-cuda12.1-rocm5.7
S := ../..
D := docker/$(T)

all:
	tar cf repo.tar Dockerfile && \
	(cd $(S) && git ls-files --recurse-submodules -z | xargs -0 tar rf $(D)/repo.tar) && \
	rm -f repo.tar.gz && gzip -1 repo.tar && \
	docker buildx build --tag charm-sycl:$(T) - < repo.tar.gz; rm -f repo.tar repo.tar.gz
