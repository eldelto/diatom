
# Table of Contents

1.  [Diatom](#org56ea477)
    1.  [Assembler](#orgbf19052)
        1.  [Comments](#org3dc5a81)
        2.  [Labels & References](#org0df4062)
        3.  [Program Entry Point](#org1aeb994)
        4.  [Macros](#org29b2c9f)
    2.  [Dictionary Layout](#org66076da)
    3.  [Preamble](#org146b245)
    4.  [Performance](#orgbe67eb2)
    5.  [Portability](#org6d08002)
    6.  [Features](#org89ef696)


<a id="org56ea477"></a>

# Diatom

A Forth dialect that focuses on portability and simplicity.


<a id="orgbf19052"></a>

## Assembler

The Diatom assembler (`dasm`) is a simplistic assembler that
translates DiatomVM instructions (`.dasm`) to their respective
opcodes (`.dopc`).


<a id="org3dc5a81"></a>

### Comments

Comments are started with `(` and ended with `)`. Please keep in
mind that both parantheses need to be surrounded by spaces or
beginning/end of line:

    ( This is a comment )
    
    ( The next function duplicates
      a given number. )
    :duplicate
      dup
      +
      ret


<a id="org0df4062"></a>

### Labels & References

The assembler supports basic labels and references (backwards &
forwards references) that can be used to reference to specific
memory locations in your program.

Labels are created with `:<name>` and can be referenced with
`@<name>`. The reference inserts the memory address of the label
as literal value:

    ( Adds the address of the label to the stack. )
    :add-own-address ( address = 125 )
      const
      @add-own-address
      ret

would resolve to

    const
    125
    red


<a id="org1aeb994"></a>

### Program Entry Point

Upon start the DiatomVM will begin executing instructions from
the first memory address. To skip directly to the entry point of
your program you can use a forward reference to a label:

    ( Immediately jump to the start label )
    const
    -1
    cjmp
    @start
    
    ( Other code that shouldn't be executed right away )
    ...
    
    ( Entry point of my program )
    :start
    ...


<a id="org29b2c9f"></a>

### Macros

The assembler has a couple of built-in macros that are expanded
as the first step in the assembly process. Most of them are
designed to easy the implementation of Forth-like languages but
can also be used in other contexts.

1.  Literals

    Literal numbers are split into consecutive bytes by the
    assembler. The number of bytes depends on the configured word
    size (default = 32 bit = 4 bytes).
    
        const
        500
    
    will be expanded to
    
        const
        0
        0
        1
        244

2.  .const

    The `.const` macro defines a constant value that cannot be
    changed at runtime. The generated instructions will be appended
    to a Forth-like dictionary but the code can also be called
    directly.
    
    Syntax: `.const <name> <numeric-value | label> .end`
    
        .const
          max-char
          255
        .end
    
    will be expanded to
    
        :max-char
          0
          8
          109
          97
          120
          45
          99
          104
          97
          114
        :_dictmax-char
          const
          0
          0
          0
          255
          ret
    
    Calling `max-char` will put the value `255` on the data stack
    of the virtual machine:
    
        call
        @_dictmax-char

3.  .var

    Variables can be defined with the `.var` macro. In contrast to
    constants, a variables value can be changed at runtime.
    
    Syntax: `.var <name> <numeric-value | label> .end`
    
        .var
          base
          10
        .end
    
    will be expanded to
    
        :base
          0
          4
          98
          97
          115
          101
        :_dictbase
          const
          @_varbase
          ret
        :_varbase
          0
          0
          0
          10
    
    Calling `base` will put the **address** of the variable's memory
    location on the data stack. This address can then be used to
    read (`@`) or update (`!`) the variable's value.

4.  .codeword

    Codewords are very specific to Forth-like languages. Codewords
    are essentially functions (a.k.a. words) consisting of assembly
    instructions that are registered in the Forth dictionary.
    
    Syntax: `.codeword <name> <instructions> .end`
    
    The codeword macro also supports a shorthand for calling other
    words from the dictionary with `!<word-name>`.
    
        .const
          1k
          1000
        .end
        
        .codeword
          add-1k
          !1k
          +
        .end
    
    will be expanded to
    
        ( 1k const )
        :1k
        0
        2
        49
        107
        :_dict1k
        const
        0
        0
        3
        232
        ret
        
        ( add-1k word )
        :add-1k
        @1k
        6
        97
        100
        100
        45
        49
        107
        :_dictadd-1k
        call @_dict1k
        +
        ret


<a id="org66076da"></a>

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
<td class="org-left">len</td>
<td class="org-right">1</td>
<td class="org-left">Length of the name.</td>
</tr>


<tr>
<td class="org-left">name</td>
<td class="org-right">n</td>
<td class="org-left">Name of the word (1 char / word).</td>
</tr>


<tr>
<td class="org-left">code-word</td>
<td class="org-right">1</td>
<td class="org-left">Pointer to the first word to execute.</td>
</tr>
</tbody>
</table>


<a id="org146b245"></a>

## Preamble

Diatom code that needs to be executed on every startup to properly
initialize the memory space. The following tasks need to be
executed:

-   Init dictionary pointer `dp`
-   Add native words to dictionary (add, dup, @, !, etc.)
-   Execute main word (default = REPL but overwritable)


<a id="orgbe67eb2"></a>

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


<a id="org6d08002"></a>

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


<a id="org89ef696"></a>

## Features

-   Exception support?
-   Tail-recursive looping
-   Integer overflow trapping vs. saturation?

