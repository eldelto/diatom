
# Table of Contents

1.  [Diatom](#org450e06b)
    1.  [Dictionary Layout](#orgb40a98a)
    2.  [Preamble](#org046e56a)
    3.  [Performance](#org3062a75)
    4.  [Portability](#org99a8aa6)
    5.  [Assembler](#org37df4f2)


<a id="org450e06b"></a>

# Diatom

A Forth dialect that focuses on portability and simplicity.


<a id="orgb40a98a"></a>

## Dictionary Layout

<table border="2" cellspacing="0" cellpadding="6" rules="groups" frame="hsides">


<colgroup>
<col  class="org-left" />

<col  class="org-right" />

<col  class="org-left" />
</colgroup>
<thead>
<tr>
<th scope="col" class="org-left">Name</th>
<th scope="col" class="org-right">Length [Word]</th>
<th scope="col" class="org-left">Comment</th>
</tr>
</thead>

<tbody>
<tr>
<td class="org-left">previous</td>
<td class="org-right">1</td>
<td class="org-left">Link to the previous word (or NULL if first word).</td>
</tr>


<tr>
<td class="org-left">name</td>
<td class="org-right">8</td>
<td class="org-left">Stores the first 4 chars of the package and the word.</td>
</tr>


<tr>
<td class="org-left">code<sub>segment</sub></td>
<td class="org-right">N</td>
<td class="org-left">Pointers to the sub-words.</td>
</tr>
</tbody>
</table>


<a id="org046e56a"></a>

## Preamble

Diatom code that needs to be executed on every startup to properly
initialize the memory space. The following tasks need to be
executed:

-   Init dictionary pointer `dp`
-   Add native words to dictionary (add, dup, @, !, etc.)
-   Execute main word (default = REPL but overwritable)


<a id="org3062a75"></a>

## Performance

Hosted on top of some other programming language (e.g. C or
Javascript) it is pretty unlikely to meet the performance of the
host language. The only thing one can do is trying to minimize the
call overhead of words as much as possible because this could
still be cheaper than constructing a complete stack frame for a
host language function call.

Apart from this the biggest performance improvement can probably
be achieved not on a instruction level but for whole applications
by keeping them as simple and lean as possible. The assumption is,
that e.g. a lean Diatom application can still be faster (by doing
much less) then a comparable Java application following "best
practices". Here any performance would of course not be gained by
just using another language but by applying different design
principles that focus on the core functionality.


<a id="org99a8aa6"></a>

## Portability

To make the implementation as portable as possible the native
Forth words are not implemented in the host language directly but
in a virtual stack machine that is suited for running stack-based
languages.

This way, only the virtual machine primitives need to be
implemented in the host language and nothing else. The drawback
is, that it takes longer to get to the first running
implementation because additional custom software (e.g. assembler)
is needed for bootstrapping.


<a id="org37df4f2"></a>

## Assembler

A very simplistic assembler is needed that can replace label
references with their actual memory location.

Input:

    double:
    	dup	
    	add
    quadruple:
    	@double
    	@double

Output:

    dup
    add
    const
    0
    next
    const
    0
    next

