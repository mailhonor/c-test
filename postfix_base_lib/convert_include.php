<?

function ccc($fn)
{
	echo $fn,"\n";
	$con = file_get_contents($fn);
	$news=Array();
	$lines = explode("\n", $con);
	foreach($lines as $line){
		if(strncmp($line, "#include <", 10)){
			$news[]=$line;
			continue;
		}
		$tmp = explode(">", substr($line, 10));
		$tf = $tmp[0];
		if(!is_file("include/$tf")){
			$news[]=$line;
			continue;
		}
		$line = str_replace("<", '"', $line);
		$line = str_replace(">", '"', $line);
		$news[]=$line;
	}
	$con = join("\n", $news);
	file_put_contents("/usr/local/include/postfix_dev/$fn", $con);

}

chdir("include");

$fs = glob("*.h");
foreach($fs as $f){
	ccc($f);
}

chdir("../");
