Data analyzer
-----------------------
Now, create a virtual environment:

    virtualenv env

With that created, you'll want to CD to a different directory and then back to the root project directory. This is to restart the Autoenv script with the environment you 
just created. Now, every time you CD into the base directory, it loads your virtual environment automatically.

First thing is to install the required libraries:

    pip install -r requirements.txt
