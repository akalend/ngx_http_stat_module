<?php

setcookie("TestYyy", "yyy_".rand(1,10)  , 0, "/yyy");

usleep(350000);
echo "set coockie Ok";