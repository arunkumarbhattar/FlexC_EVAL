<html>
<head>
<title>Welcome to the PHP Index</title>
<style>
#left {
float: left;
width: 210px;
border-right: 5px dashed #369;
}
#center {
margin-left: 240px;
padding: 10px;
}
</style>
</head>
<body>
<div id="left">
<img src="favicon.ico" width=200 height=200>
</div>
<div id = "center">
<h1>Index . PHP</h1>
<i>Homepage of a website</i>
<hr>
Welcome to this test page, served by CoreHTTP! If you see this it means the
web server is working properly. Thank you for trying out this software.
<br><br>
Thank you a bunch.
<hr>
<ul>
<li>view the dynamic sleeper php page <a href="test.php">test.php</a>
<li>view the dynamic perl page <a href="index.pl">index.pl</a>
<li>view the html index page <a href="index.html">index.html</a>
<li>view a nonexistent page <a href="asdfadsfa">asdfasdfasdd</a>
<li>view the image <a href="favicon.ico">favicon.ico</a>
<li>view the image <a href="benchmark.jpg">benchmark.jpg</a>
<li>view the directory <a href="folderwith.extension">folderwith.extension</a>
</ul>
<?php
for ($i = 0; $i < 5; $i++) echo "look, a simple php script! $i<br>";
?>
</center>
</body>
</html>


