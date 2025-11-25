DEFINES+=PROJECT_CONF_H=\"project-conf.h\"

### Projecto que iremos compilar:
CONTIKI_PROJECT = Example Projeto

### Drivers de sensores que necessitamos (tipicamente contidos em
### ~/contiki/platform/zoul/dev). Remover # se necessário:
CONTIKI_TARGET_SOURCEFILES += dht22.c
### Quando fizermos "make", iremos compilar todos os projectos mencionados acima:
all: $(CONTIKI_PROJECT) 

### Localização da pasta root "contiki" em relação à pasta actual: 
TARGET = zoul
CONTIKI = ../../contiki/
include $(CONTIKI)/Makefile.include