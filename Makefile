
all: 
	echo "Building all Trafodion components"
	cd core && $(MAKE) all 

package: all
	echo "Packaging Trafodion components"
	cd core && $(MAKE) package 

package-all: package
	echo "Packaging all Trafodion components"
	cd core && $(MAKE) package-all 

package-src: 
	echo "Packaging source for all Trafodion components"
	mkdir -p distribution
	git ls-files  |tar -cf distribution/trafodion-src.tar.gz -T -

eclipse: 
	echo "Making eclipse projects for Trafodion components"
	cd core && $(MAKE) eclipse 


clean: 
	echo "Removing Trafodion objects"
	cd core && $(MAKE) clean 

cleanall: 
	echo "Removing all Trafodion objects"
	cd core && $(MAKE) cleanall 
