## Cast Checker

<img src="arh_diagram.PNG" alt="Cast Checker's Architecture" style="width:1280px;"/>

Developers relax restrictions on a type to reuse methods with other types. While type casts are prevalent, in weakly typed languages such as C++, they are also extremely permissive. Assignments where a source expression is cast into a new type and assigned to a target variable of the new type, can lead to software bugs if performed without care. In this paper, we propose an information-theoretic approach to identify poor implementations of explicit cast operations. Our approach measures accord between the source expression and the target variable using conditional entropy. We collect casts from 34 components of the Chromium project, which collectively account for 27MLOC and random-uniformly sample this dataset to create a manually labelled dataset of 271 casts. Information-theoretic vetting of these 271 casts achieves a peak precision of 81% and a recall of 90%. We additionally present the findings of an in-depth investigation of notable explicit casts, two of which were fixed in recent releases of the Chromium project.