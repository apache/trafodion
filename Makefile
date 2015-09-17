
all: 
	echo "Building all Trafodion components"
	cd core && $(MAKE) all 

package: all
	echo "Packaging Trafodion components"
	cd core && $(MAKE) package 

package-all: package
	echo "Packaging all Trafodion components"
	cd core && $(MAKE) package-all 

clean: 
	echo "Removing Trafodion objects"
	cd core && $(MAKE) clean 

cleanall: 
	echo "Removing all Trafodion objects"
	cd core && $(MAKE) cleanall 
