
# Table of Contents

1.  [Diatom](#org856f220)
    1.  [Code Layout](#orga759922)
    2.  [Preamble](#org58948df)
    3.  [Performance](#org31bf7e2)


<a id="org856f220"></a>

# Diatom

A Forth dialect that focuses on portability and simplicity.


<a id="orga759922"></a>

## Code Layout

<table border="2" cellspacing="0" cellpadding="6" rules="groups" frame="hsides">


<colgroup>
<col  class="org-left" />

<col  class="org-left" />
</colgroup>
<thead>
<tr>
<th scope="col" class="org-left">Code Layout</th>
<th scope="col" class="org-left">Comment</th>
</tr>
</thead>

<tbody>
<tr>
<td class="org-left">Next word pointer</td>
<td class="org-left">&#xa0;</td>
</tr>


<tr>
<td class="org-left">Word name</td>
<td class="org-left">Stores the first 4 chars of the package and the word.</td>
</tr>


<tr>
<td class="org-left">Code segment</td>
<td class="org-left">&#xa0;</td>
</tr>


<tr>
<td class="org-left">Parameter segment</td>
<td class="org-left">&#xa0;</td>
</tr>
</tbody>
</table>


<a id="org58948df"></a>

## Preamble

Diatom code that needs to be executed on every startup to proberly
initialize the memory space. The following tasks need to be
executed:

-   Init dictionary pointer `dp`
-   Add native words to dictionary (add, dup, @, !, etc.)
-   Execute main word (default = REPL but overwritable)


<a id="org31bf7e2"></a>

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

