# Warning: This language is still being designed and is in a very early alpha stage.

# Web Front-End Language (FEL)

FEL (working title) is a work-in-progress language aiming to make working HTML, CSS and JavaScript less painful and increase
productivity by creating simple underlying abstractions and allowing full introspection of HTML/CSS so that browser-specific bugs can
be caught with user-code and so that CSS rules can be optimized to run faster in the browser without losing readability.

# Modular HTML Components

```
def Banner
{
	layout
	{
		div(class = "banner")
		{
			div(class = "banner-inner")
			{
				children
			}
		}
	}
}
```

```
def Link
{
	properties
	{
		src = "",
	}

	layout
	{
		// NOTE: 'when' keyword means that element won't wrap if 'src' evaluates false.
		a(href = src) when src
		{
			children
		}
	}
}
```

# Template Code

```
layout
{
	div(class = "container")
	{
		Link(src = "http://www.google.com.au")
		{
			"My Link"
		}
		Link(src="")
		{
			p
			{
				"My paragraph text"
			}
			Banner()
			{
				img(src = "http://mycoolimage.com/image.jpg")
			}
		}
	}
}
```

Output:
```
<div class="container">
	<a href="http://www.google.com.au">
		My Link
	</a>
	<p>
		My paragraph text
	</p>
	<div class="banner">
		<div class="banner-inner">
			<img src="http://mycoolimage.com/image.jpg"/>
		</div>
	</div>
</div>
```

# Interfacing with backend code in a template

```
layout
{
	div(class = "container")
	{
		Link(src = $Link)
		{
			"My Link"
		}
		if ($Link + ".jpg" == "http://mycoolimage.com/image.jpg")
		{
			Banner()
			{
				img(src=$Link + ".jpg");
			}
		}
	}
}
```

Output in PHP:
```
<div class="container">
	<?php if ($Link): ?><a href="<?php echo $Link; ?>"><?php endif; ?>
		My Link
	<?php if ($Link): ?></a><?php endif; ?>
	<?php if ($Link.'.jpg' === 'http://mycoolimage.com/image.jpg'): ?>
		<div class="banner">
			<div class="banner-inner">
				<img src="<?php echo $Link.'.jpg'; ?>"/>
			</div>
		</div>
	<?php endif; ?>
</div>
```

# Introspection rule

```
introspect 
{
	if (target.IE <= 9)
	{
		for (html as element)
		{
			if (element.css('position') == 'absolute')
			{
				if (element.css('z-index') == null) // instead of 'null' use 'notset' keyword? as the value isnt set.
				{
					compiler_error('For IE6-9 you must use z-indexes explicitly or things will break.');
				}
			}
		}
	}

	// Remove all un-used CSS rules in HTML
	// Alternatively you could make the compiler throw a warning maybe to avoid unnecessary
	// code entering production.
	for (css as selector)
	{
		if (!html.find(selector))
		{
			css.remove(selector);
		}
	}
}