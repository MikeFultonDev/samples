set -o emacs
`df | grep -q "/var/zpm"` 
mvar=$?
`df | grep -q "/usr/lpp/IBM/zpm"`
mcode=$?
if [ $mvar -gt 0 ] || [ $mcode -gt 0 ]; then
	echo "Need to have IBMUSER mount ZPM file systems" >&2
	echo "Environment not established" >&2
else
	export PATH=/usr/lpp/IBM/zpm/python/3.10.0.0/bin:$PATH
	export PATH="/usr/lpp/IBM/zpm/node/16.0.0.1/bin:$PATH"
	export PATH=/usr/lpp/IBM/zpm/opencpp/1.0.0.0/bin:$PATH
	export PATH=/usr/lpp/IBM/zpm/zoau/1.2.0.1/bin:$PATH
	export PATH=$JAVA_HOME/bin:$PATH
	export LIBPATH=/usr/lpp/IBM/zpm/python/3.10.0.0/lib:$LIBPATH
	export LIBPATH=/usr/lpp/IBM/zpm/zoau/1.2.0.1/lib:$LIBPATH
	export PYTHON=/usr/lpp/IBM/zpm/python/3.10.0.0/bin/python3
	export JAVA_HOME=/usr/lpp/IBM/zpm/java/8.0.7.0/J8.0_64
	export ZOAU_HOME=/usr/lpp/IBM/zpm/zoau/1.2.0.1

	export npm_config_nodedir="/usr/lpp/IBM/zpm/node/16.0.0.1"
	export npm_config_zoslib_include_dir="/usr/lpp/IBM/zpm/node/16.0.0.1/include/node/zoslib"
	export MANPATH=/usr/lpp/IBM/zpm/zoau/1.2.0.1/docs/%L:$MANPATH

	if [ "${_BPXK_AUTOCVT}" != "ALL" ]; then
	  export _BPXK_AUTOCVT=ON
	fi
	export _CEE_RUNOPTS="$_CEE_RUNOPTS FILETAG(AUTOCVT,AUTOTAG) POSIX(ON)"
	export _TAG_REDIR_ERR=txt
	export _TAG_REDIR_IN=txt
	export _TAG_REDIR_OUT=txt

	export PATH=/usr/lpp/IBM/zpm/package-manager/bin:$PATH

	. /usr/lpp/IBM/zpm/golang/1.17.0.1/etc/envsetup
fi
