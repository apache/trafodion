all:Trafci_deploy
Trafci_deploy:Trafci_installer
Trafci_installer:Trafci_jar
Trafci_jar:
	mvn clean install$
Trafci_installer:
	./getBuildInfo$
	export buildId=`head -n 1 buildId`$
	mvn -f installer_pom.xml -DbldId=${buildId} clean package$
Trafci_deploy:
	java -jar target/trafciInstaller.jar -silent -jdbcFile ${MY_SQROOT}/export/lib/jdbcT4.jar -installDir ${MY_SQROOT}
