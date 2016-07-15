# Warning: This language is still being designed and is in a very early alpha stage.

# Web Front-End Language (FEL)

FEL (working title) is a work-in-progress language aiming to make working HTML, CSS and JavaScript less painful. Increasing productivity by creating simple underlying abstractions. FEL allows full introspection of HTML/CSS, thus browser-specific bugs can be caught with user-code and CSS rules can be optimized to run faster in the browser without losing readability.

# Modular HTML Components

```cpp
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

	css 
	{
		// Poor example of actual CSS but oh well.
		.banner {
			position: relative
			z-index: 0
			width: 100%
			max-width: 100%
		}

		.banner-inner {
			width: 100%
			max-width: 1920px
		}
	}
}
```

```cpp
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

```cpp
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
```php
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

The idea here is that any variable or function beginning with $ will generate the appropriate code based on a backend of your selection. Whether it be PHP, Ruby, JavaScript etc.

I only have experience working with PHP templates however, so I'm not sure how implementing other languages would go.

```cpp
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
```php
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

The idea behind this is to allow a user to iterate over every HTML element in a flat list, check computed CSS values and take an appropriate action, such as throw an error, modify/remove the element, etc.

The user could also iterate over each CSS rule, check the selectors of each rule and remove selectors/rules that don't have any matching HTML.

```cpp
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
	for (css as rule)
	{
		for (rule as selector)
		{
			if (!html.find(selector))
			{
				rule.remove(selector)
			}
		}
		if (len(rule) == 0)
		{
			css.remove(rule);
		}
	}
}