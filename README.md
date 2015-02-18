# Warning

I do not guarentee this is good. Also I misuse the term signature in many places, but what I use `signature` to denote is the parameters to a function and possibly the return value. If you don't like my misuse of the term then I'm sorry, don't use my docs then.

# Parser

This is a parser that I made that isn't necessarily good or stable, state of the art or usable. Anyways what follows is an outline for how it works:

## How it Works

- Tokens: A token is some object that you want to be considered its own *thing*. For instance if you were makeing a json parser you would have `{`, `}` tokens and so on. Tokens have a type - they are either sequences (which are some special sequence of characters), containers (tokens which contain other tokens), unknown (meaning the type hasn;t been established yet) or custom (which isn't currently used but in theory allows extensibility of this parser).
- Tokenizer: A tokenizer is an object which tries to recognize a token. Basically it wraps a token and has actual logic for checking data against that token.
- Tokenizer Table: Each tokenizer is represented by a `size_t` (aka `tokenizer_id`). This table manages these ids. You contact this table to register your tokenizer (and thus your token) and then give the `tokenizer_id` to the lexer, which will be discussed below. Why you pass around `tokenizer_id`s instead of just some sort of tokenizer object will be discussed below.
- Lexer: A lexer uses an array of tokenizers to tokenize text. You feed it a string and it will give you back an array of tokens.


## Specifics to Classes

Here I will discuss specific members and such. First I want to explain the `prop` macros. These try to act like `objc`s `@property`. The ending after `prop` denotes what kind of property it will be. These properties automatically create a private variable with `_` prepended to the variable name you give it. The basic property has signature of `(type, name)` meaning you give it the type of the property you want and the name you want it to have. Here are the different types of properties you can make and information about them:
- `prop(type, name)`: This is just a normal property. It creates a private variable called `_name` where `name` is the `name` fed in when creating the property. Next it creates two getter methods. The first has signature `type&()` meaning it will return an object with type `type&` and takes no paramteters. The second getter is for const objects so it has sig `type const&() const` so that you can still get this property when working with a const object. The last function added is a setter with sig `auto&(type)` meaning it will take in the type and set the internal state to the value you pass in. It will then return a reference to the current object you will be calling this on.
- `propro(type, name)`: This is a **R**ead **O**nly property. You only have the second function described above for `prop` - the one where you get back a constant reference to the object. Of course theres a private member for setting the value of this object too.
- `props(type, name, setter)`: This is a **S**etter property, also known as a property with a custom setter. It has all the same methods as a `prop` but inside the setter function (the last function described above) your `setter` code will be ran. The input parameter to this setter code is called `val` and has the type of `type` as expected. You can modify that `val` or simply use it to modify other state.
- `propg(type, name, getter)`: This is a **G**etter property, also known as a property with a custom getter. It has all the same methods as a `prop` but inside the getter functions (the first two functions described) your custom code will be ran. The value that will be returned is called `out` and has type `type`. You can modify `out` so that it will return a different object, but note as of now this must still have type `type`.
- `propsg(type, name, setter, getter)`: This is a **S**etter **G**etter property, in other words a combination of the setter and getter properties. You have custom setters and getters here as described above for the setter and getter properties repsecitively. 
- `proprog(type, name, getter)`: This is a **R**ead **O**nly **G**etter property, meaning a read only property with a custom getter. Read about the ro and getter properties to understand what this type of property does.

### `token`

#### Overview

This is really just a container class that has a lot of code for describing what this token looks like.

#### Properties

- `type :token_type`: This represents the type of token this is. There is an enum that has 4 members: `sequence`, `container`, `unknown` and `custom`. Currently there are no custom tokens. Unknown is to represent a token that hasn't been fully initialized. A sequence token is a token that will contain a sequence of characters. A container token will contain other tokens.
- `type_description :char*`: This describes what the type of token is. For instance a container token with `{` and `}` as the beginning and ending of the token might have a `type_description` of `dictionary` or something of the sort (I'm thinking of json here).
- `data :char*`: This is the data that is actually behind this given token. So if my token represents a the ASM instruction `pushd` then the data will always be `pushd`. Some tokens will have non-static `data` because they are in some sense dynamic tokens. For example a token could be a sort of regex for a three character string containing only abc, i.e. `[abc]{3}`.
- `id :token_id`: This is a an id that will be internally given to this token after you register it with the `tokenizer_table` through a `tokenizer`.

#### Ivars

- `_description :char*`: This is an internal variable that is used when creating the description of a given token. This is used if you want to print what the tokens are - I currently use this a lot for debugging and testing.

#### Functions

- constructors: Of course you have your constructors, there is one that takes no parameters and will give you basically an empty token and then another that takes the `token_type` and sets the type property accordingly. Currently the second of these constructors is what is mainly used.
- `char *desicription()`: This is how you actually get the description of this token.
- `auto& append(char c)`: This allows one to append data to a token. It will return a reference to the object you called it on. Basically this is a convenience method for intereacting with the `data` property.

### `tokenizer`

#### Overview

This is a class which wraps a `token` object and has logic for propcessing and creating tokens. In other words a lexer would create these tokenizers and then check against each of these tokenizers if it can successfully create a token. If it can it asks the tokenizer to create a token and then the lexer stores the tokens in some large list of tokens somewhere. Your tokenizer class is responsible for checking if the sequence of data fits the given token, actually creating and filling out tokens and being magical. In my opinion this is where the real work of your parser is.

#### Properties

- `tok :token*`: This is a pointer to the current `token` that the tokenizer is trying to process. This `tok` isn't meant to be dealt with externally, instead one should use the functions `create_token`, `reset` and the like to modify this token.
- `type_description :char*` This is a character array that is the type decription for each token. You set it here and then from here on out when a token is created by the tokenizer it will set the proper `type_description` on it.
- `validator :std::function<bool(token*)>`: This is a validator that checks if the token is properly created. More on this later.
- `tokid :token_id`: This is the `token_id` as given by the `tokenizer_table`. This is used internally and probably won't need to be messed with.

#### Functions

- constructor: There is one constructor that intializes everything to NULL. This is to be overridden.
- `token* create_token()`: This is meant to be overriden by a subclass. Here you should create an empty token that this tokenizer wraps. You should simply return said token, don't set the `tok` property here.
- `token* reset()`: This shouldn't have to be overriden but is able to be. By default it will return the previous token and then create a new token with the proper `type_description` and `id` set and sets the property `tok` to this new token. This is sort of a `pop` method that returns the old token a creates a fresh new token for the tokenizer to use.
- `bool add_character(char c)`: This function should be implemented by a subclass and checks to see if a character can be added. Of course for container tokens this gets a bit tricky, but more on that later.
- `bool full()`: This should be implemented by a subclass to return whether or not a token has been fully recognized. For variable length tokens this gets tricky, more on that later.
- `size_t lex(const char *data)`: This should be implemented by a subclass but does actual lexing of data. If it cannot create a token from the data given it return `0` otherwise it returns how many characters it used up when lexing. This is how variable length tokens (like containers) are dealt with. What happens is the lexer asks the tokenizer to attempt to tokenizer a piece of data, it tries to, and if it fails then it goes on try a new tokenizer until its list of tokenizers is exhausted. Even if this does not finish recognizing a token you should return how far you got. Then you denote that it isn't finished by return `false` in your `full()` method. Look to `strict_sequence_tokenizer` as an example.

### `tokenizer_table`

#### Overview

This isn't actual a class but a namespace. It controls creation of tokenizer ids and the such. The reason we use `tokenizer_ids` instead of simply passing around tokenizers is because if we have a container which should be able to contain itself. For example if we have a dictionary container, it should be able to contain other dictionaries. This would then require an infinite loop of tokenizers to be created (since a container has its own set of tokenizers so that it can tokenize the data it contains). Instead we give `tokenizer_id`s so that when it needs to it can create a tokenizer. In other words this is lazy allocation of tokenizers.

#### Types

- `tokenizer_creator`: This is a function that takes in a `tokenizer_id` and returns a `tokenizer`. Basically its a lambda that actually creates the tokenizer represented by the `tokenizer_id`.

#### Functions

- `tokenizer_id register_tokenizer_creator(tokenizer_creator)`: This is how you register a tokenizer. You give it a lambda that creates the tokenizer and it gives back the `id` assocaiated to said tokenizer creator.
- `tokenizer_id register_tokenizer_creator(tokenizer_id, tokenizer_creator)`: This is how you register a creator with a specific `id` in mind. This can be useful when creating container tokenizers. You might want to create a container that can contain itself and so you need to give the tokenizer the `tokenizer_id` for the container itself when you create it, but you don't know the `tokenizer_id` until after you register this with the `table`. One way to eliminate this is specify what the `id` of the container `tokenizer` should be. Another way is to create a variable outside your lambda creator that represents the `tokenizer_id` and is then set after you register the creator with the `table`. If you don't understand this issue try registering container token that can contain itself and you'll see the issue.

### `strict_sequence_tokenizer`

#### Overview

This is a tokenizer which will only match the the sequence of characters you give to the constructor literally. No regex, no fancy shmanciness, just the sequence you give it.

### `sequence_tokenizer`

#### Overview

This is a more flexible sequence tokenizer. Instead of only allowing the exact sequence you give it, instead you give characters that are allowed and how long its allowed to be. By default the `min_length` (the smallest its allowed to be) is set the the `max_length` the longest its allowed to be. But this can be changed. For example if you want a tokenizer representing `[abc]{1,8}` you would create a `sequence_tokenizer` with `allowed_characters` of `"abc"` and `len` of `8` and then set `min_length` to `1`.

#### Properties

- `allowed_characters :const char*`: This is a character array of allowed characters. This is set by the constructor and should never be tampered with again (hence why it is const and read only).
- `max_length :size_t`: This is how long your sequence is allowed to be. This is set by the constructor but allowed to change later.
- `min_length :size_t`: This is the minimum length that needs to be eaten up by this tokenizer in order to be considered `full`. 

#### Functions

- `sequence_tokenizer(const char *allowed, size_t len)`: This is the main constructor that sets `allowed_characters` to `allowed` (note that this is `const char*` meaning this object will not be responsible for releasing this character array). It will also set `max_length = min_length = len`. 


### `istr_tokenizer`

This is yet another sequence tokenizer, but this time this is a tokenizer to be used inside a string container tokenizer. It will only accept ascii values in the range of `[' ', '~']` and nothing outside of it. It will properly take care of escapes of the `"` character (meaning if it was escaped then it will add in the `"` character otherwise it won't. This is for a string that's contained with `"` characters only. In the future this might be configurable (TODO: make `istr_tokenizer` configurable to what the token specifying what the end of a string is).

### `seqtoker`s: `lalpha_tokenizer`, `ualpha_tokenizer`, `alphai_tokenizer`, `numeric_tokenizer`, `lalphanumeric_tokenizer`, `ualphanumeric_tokenizer`, `alphainumeric_tokenizer`, `space_tokenizer`, `whitespace_tokenizer`, `ascii_tokenizer`

These are all subclasses of `sequence_tokenizer` with the `allowed_characters` already specified by the macros second argument. The first argument specificies the name of the tokenizer (so if you specify `foo` then it will make a class called `foo_tokenizer`) and the second will be the argument sent to the `sequence_tokenizer` constructor.

### `number_tokenizer`

This is even another sequence tokenizer, but this time its special for parsing numbers. It will parse things like `5.00`, `5e3`, `-5.01`, `5e-3` and so on. This class really shows one of the design principles used in this library. We want the tokenizers to *ONLY* attempt to make a token. It does not make a number out of this data or try to interpret this data. It simply does checks to see if this data looks like a number. You can look through the functions defined in this class yourself, they are somewhat verbose, so I will not go though them here.

### `lexer`

This is the big kahuna. This is the class that actually takes a string of some sort and tokenizes it. You give the lexer a list of `tokenizer_id`s and it will, as it needs to, create the tokenizers and attempt to parse the text you gave it.

#### Properties

- `tokens :std::vector<token*>`: This is an array of tokens that the lexer recognized. Once lexing is done you access the tokens via this property. These can only be set internally hence their readonly-ness.
- `tokenizers :std::vector<tokenizer_id>`: This is an array of `tokenizer_id`s you want the lexer to use to try and tokenize your text. Order matters here, the first one in the array will be the first one attempted to be used. these can only be set by the functions provided and the constructor, so only use them.
- `current_character :char`: This is the character that the lexer is currently analyzing. Note this is not up to date if you interrupt it while a tokenizer is doing its thing. In other words this is the current character that a tokenizer is trying to start at to detect a token.
- `current_index :size_t`: This is the index corresponding to the `current_character`.
- `finished :bool`: You use this to see if the lexer is finished. If it isn't that means there was an error (it failed to find a token at some given `current_index`).
- `verbose :bool`: Set this if you want the lexer to let you know what he/she's doing (you allowed to name your parser anything except Pedro. If you name it Pedro there will be **SERIOUS** consequences).

#### Functions

- Constructors: There is of course the constructor that initializes an empty lexer. Then there are the constructors that initialize it will specific tokenizers you want it to use (in the form of `va_args` of `tokenizer_id`s or simply a `tokenizer_id_list` aka `std::vector<tokenizer_id>`).

- `lexer& add_tokenizer(tokenizer_id, ...)`: You can use this to add more `tokenizer_id`s to your lexer. Unfortunately as of now you cannot reorder the tokenizers (TODO: allow reordering of tokenizers for a given lexer).
- `lexer& add_tokenizer(tokenizer_id_list)`: This does the same thing as the above `va_args` variant but does so with a specified array instead.
- `lexer& reset()`: This resets the lexer. It clears the tokens array, sets it to not finished and starts over. This lets you lex multiple pieces of text back to back to back.
- `lexer& lex(const char *data)`: This is it, the function you've been waiting for. Feed this guy a chunk of text (you are responsible for allocation of said text hence the `const`) and it will try to tokenize it. Once this method returns you can call `finished()` and the like to analyze if this guy was successful. The lexer checks if the current tokenizer its analyzing can be used to tokenize the text by initially calling `add_character` on it and seeing if it accepts that one character. If it does then it calls the `lex` function on the tokenizer to create the token(s), otherwise it moves on to another tokenizer and tries the same thing. `tokenizer`s should just throw away characters which they do not accept in order to not mess up their internal state.

### `container_token`

This is a token that represents a container of some sort. It will contain other tokens as it sees fit. This is still simply a container, but now it also contains other `token` objects. The logic for adding these token objects is still in the `tokenizer` corresponding to this token.

#### Properties

- `tokens :std::vector<token*>`: This is an array of tokens that this container contains. Read only of course because you should only use the functions below to mutate it.
- `begin, end :char`: These are the characters that specify the beginning and ending of your container (unfortunately currently we only support containers that begin and end with only one character as opposed to perhaps a `token` for each the beginning and the end) (TODO: Make the beginning and ending of a container specified by a token).

#### Functions

- Constructor: This is straight forward, it initializes all the ivars and properties properly. There is only one that doesn't take any arguments (this is for design purposes).
- `container_token& boundary(char, char)`: This specifies the beginning and end of the container. The first argument is the beginning character and the second is the ending.
- `container_token& addTokens(token_list)`: This allows the `tokenizer` to add tokens to this container. It takes `token_list` aka `std::vector<token*>`.

### `container_tokenizer`

This is the `tokenizer` that wraps the `container_token`s. You specify the boundary of the container and what tokenizers are allowed to be used to recognize tokens inside the container and then the magic happens. What actually happens is a `lexer` object is created and used to do the actual tokenizing of the inside of the container once. This is the most *beta* class currently and I'm unsure on the stability of it.

#### Properties

- `begin, end :char`: This is the boundary for the `container_token` that it creates. These are read only because you should only use the functions below to modify them.
- `tokenizers :std::vector<tokenizer_id>`: This is the list of tokenizers that were allowed to use for tokenizing the text inside the container. You should only use the methods below for modifying this list.

#### Functions

- Constructors: You must specifiy the boundary when constructing a `container_tokenizer`, then you can specify any number of `tokenizer_id`s to be used when tokenizing the inside of the container.

- `container_tokenizer& add_tokenizer(tokenizer_id)`: Add a single tokenizer to the list of tokenizers used when tokenizing the inside of the container.
- `container_tokenizer& add_tokenizers(tokenizer_id,...)`: Add as many tokenizers as you want to the list of tokenizers used when tokenizing the inside of the container.
- `container_tokenizer& add_tokenizers(std::vector<tokenizer_id>)`: Same as above but with an array instead of `va_args`.

---

Lets take a break from all that documentation. What we have now done is tokenized the data that has come in. The general way one will go about tokenizing is by registering `tokenizer_creator`s with the `tokenizer_table`, feeding these `tokenizer_id`s that the table returns to the lexer and then having the lexer tokenize the text you give it. This is pretty cool, but now that we have tokens, can we make this into some meaningful data? Can we check that these tokens are organized in a proper way? As of right now the string `"hello world"{ "hello", "world" }` and the string `{ "hello", "world" }` would both pass our theoretical json parser, but only the second string is valid json. So we need to have some sort of grammar to check if these tokens are in the right place (grammar is used here for the same reason it is in english. The sentence 'hi, how are you?' has proper grammar whereas 'you are hi, how?' does not, but both have valid tokens).

Now back to the parser!

---

### `grammar_component`, `grammar`

#### Overview

This represents a piece of your grammar. It says what other tokens are allowed before it and what are allowed after it. Currently this and the `grammar` class are the least designed classes (basically I just wrote some stuff and it kinda worked sometimes). I know there should be more context awareness in these components, for example does it need a token after it, what about before it (can it be at the beginning and the end of a string?), there should be pieces of grammar composed of other pieces of grammr (so for example a function should be a piece of grammar that contains grammar for the arguments and whatnot). In general there is a big TODO: context awareness on this class. Since this is so underdesigned I am not going to write very much documentation on it.

