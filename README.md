# Swine
Swine Integrated Control, Smart farm


webapi.zip 압축해제후 /var/www/html 아래에 해제한다. 
- Codeigniter 사용을 위한 설정 
	apache2 mod_rewrite 활성화 	
	pi@raspberry:~#  sudo a2enmod rewrite
	pi@raspberry:~#  sudo service apache2 restart

	pi@raspberry:~#  nano /etc/apache2/sites-enabled/000-default.conf 아래의 내용 추가
	<Directory /var/www/html> 
	Options Indexes FollowSymLinks MultiViews 
	AllowOverride All 
	Order allow,deny 
	allow from all 
	</Directory>
 
 
 mysql 농장번호 변경
 
 root@raspberrypi:~ # mysql
	Welcome to the MariaDB monitor.  Commands end with ; or \g.
	Your MariaDB connection id is 40118
	Server version: 10.3.17-MariaDB-0+deb10u1 Raspbian 10
	
	Copyright (c) 2000, 2018, Oracle, MariaDB Corporation Ab and others.
	
	Type 'help;' or '\h' for help. Type '\c' to clear the current input statement.
	
	MariaDB [(none)]> use tcprelay;
	Reading table information for completion of table and column names
	You can turn off this feature to get a quicker startup with -A
	
	Database changed
	MariaDB [tcprelay]> UPDATE `TE_ICT_CONFIG` SET `FARM_NO`='1387',`IC_NO`='1',`IC_IP`='172.30.1.45' WHERE 1 ;
	Query OK, 0 rows affected (0.006 sec)
	Rows matched: 1  Changed: 0  Warnings: 0
  
  3. json사용을 위한 패키지 설치
  
  https://github.com/json-c/json-c  설치
  
  wget curl.haxx.se/download/curl-7.65.1.tar.gz   다운로드 설치
  
  
	tcpman.cpp 소스내에   ‘eth0’  이더넷 디바이스명칭 확인 후 수정
	
	
 4. 자세한 설치 참고사항은 개발자가이드(hangul) 참고 
  
  
  
  
  
  
