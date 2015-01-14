<?php

setcookie("TestRoot", "1234567890", 0, "/");
setcookie("TestXxx", "xxx123", time() + 3600, "/xxx" );
setcookie("TestYyy", "yyy321", 0, "/yyy");

echo "set coockie";