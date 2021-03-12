CC := gcc
CXX := g++
CFLAGS := -Wall -O3 -std=gnu99
CXXFLAGS := -Wall -O3 -std=c++11
CPPFLAGS :=  -Iinc -MMD -MP
LDFLAGS := 
LDLIBS := 

.phony: all clean

all: bin/TAP_LIME

clean: 
	 find . -name \*.o -delete
	 find . -name \*.d -delete
	 $(RM) -r obj

bin/TAP_LIME: $(patsubst %.cc,%.o,$(wildcard src/*.cc)) obj/cpu0l1iprefetcher.a obj/cpu0l1dprefetcher.a obj/cpu0l2cprefetcher.a obj/cpu0branch_predictor.a obj/cpu0btb.a obj/llprefetcher.a obj/llreplacement.a obj/l1ireplacement.a
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)

obj/cpu0l1iprefetcher.a: $(patsubst %.cc,%.o,$(wildcard prefetcher/TAP/*.cc))
	@mkdir -p $(dir $@)
	ar -rcs $@ $^

obj/cpu0l1dprefetcher.a: $(patsubst %.cc,%.o,$(wildcard prefetcher/no_l1d/*.cc))
	@mkdir -p $(dir $@)
	ar -rcs $@ $^

obj/cpu0l2cprefetcher.a: $(patsubst %.cc,%.o,$(wildcard prefetcher/no_l2c/*.cc))
	@mkdir -p $(dir $@)
	ar -rcs $@ $^

obj/cpu0branch_predictor.a: $(patsubst %.cc,%.o,$(wildcard branch/bimodal/*.cc))
	@mkdir -p $(dir $@)
	ar -rcs $@ $^

obj/cpu0btb.a: $(patsubst %.cc,%.o,$(wildcard btb/basic_btb/*.cc))
	@mkdir -p $(dir $@)
	ar -rcs $@ $^

obj/llprefetcher.a: $(patsubst %.cc,%.o,$(wildcard prefetcher/no_llc/*.cc))
	@mkdir -p $(dir $@)
	ar -rcs $@ $^

obj/llreplacement.a: $(patsubst %.cc,%.o,$(wildcard replacement/lru_llc/*.cc))
	@mkdir -p $(dir $@)
	ar -rcs $@ $^

obj/l1ireplacement.a: $(patsubst %.cc,%.o,$(wildcard replacement/lime_l1i/*.cc))
	@mkdir -p $(dir $@)
	ar -rcs $@ $^

obj/%.o: */%.c
	@mkdir -p $(dir $@)
	$(CC) -c $(CPPFLAGS) $(CFLAGS) -o $@ $<

obj/%.o: */%.cc
	@mkdir -p $(dir $@)
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) -o $@ $<

-include $(wildcard prefetcher/*/*.d)
-include $(wildcard branch/*/*.d)
-include $(wildcard btb/*/*.d)
-include $(wildcard replacement/*/*.d)
-include $(wildcard src/*.d)
