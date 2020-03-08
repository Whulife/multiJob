BUILDIR = ./build
SRCDIR = ./src
OBJDIR = ./obj

target := $(BUILDIR)/libmultiJob.so

all:$(target)

sources := $(wildcard $(SRCDIR)/*cpp)
objects = $(addprefix $(OBJDIR)/,$(patsubst %.cpp, %.o,$(notdir $(sources))))

INCDIR += -I./include

#LIBS +=

#链接选项 动态库编译及依赖库
LDFLAGS += -shared

CC := g++
RM := rm -rf


#编译选项
CCFLAGS = $(INCDIR) -g -pthread -std=c++11 -fPIC -DDEBUG 


# 终极目标规则，生成可执行文件
$(target) : $(objects)
	@if [ ! -d $(BUILDIR) ]; then mkdir -p $(BUILDIR); fi;\
	$(CC) -o $@ $^ $(LDFLAGS) $(LIBS)

$(OBJDIR)/%.o : $(SRCDIR)/%.cpp
	@if [ ! -d $(OBJDIR) ]; then mkdir -p $(OBJDIR); fi;\
	$(CC) -c $(CCFLAGS) -o $@ $<

#clean规则 
.PHONY:
clean:
	@$(RM) $(OBJDIR)

cleanall:
	@$(RM) $(BUILDIR) $(OBJDIR)
