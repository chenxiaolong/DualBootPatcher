# The blob of text

```java
    This file is part of the RootShell Project: https://github.com/Stericson/RootShell
 
    Copyright (c) 2014 Stephen Erickson, Chris Ravenscroft
 
    This code is dual-licensed under the terms of the Apache License Version 2.0 and
    the terms of the General Public License (GPL) Version 2.
    You may use this code according to either of these licenses as is most appropriate
    for your project on a case-by-case basis.

    The terms of each license can be found in the root directory of this project's repository as well as at:

    * http://www.apache.org/licenses/LICENSE-2.0
    * http://www.gnu.org/licenses/gpl-2.0.txt
 
    Unless required by applicable law or agreed to in writing, software
    distributed under these Licenses is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See each License for the specific language governing permissions and
    limitations under that License.
```

# Apache + GPL?

This is so that projects that wish to use RootShell without sharing their source code can do so.

After all, our main goal is to provide a library that can be used by any project, in a consistent manner.

# Why not just Apache, then?

This would almost have been possible: GPLv3 can be seen as a superset of the Apache v2 license. However, most GPL projects are licensed under the terms of GPLv2.
GPLv2 is NOT compatible with the Apache v2 license: some of its provisions are not consistent with the GPL's primary goal to not add any restrictions to the original license.

Therefore, to allow both GPLv2 and other types of projects to use RootShell, we had to release RootShell under both licenses.

# Which GPL license version may I use?

GPLv2 or v3.

# Is Apache the appropriate license for a commercial product?

Not necessarily. On the one hand, Apache reads more like a contract, with language about indemnification, licenses, etc. On the other hand, GPL is also compatible with commercial distribution. There is a misconception that GPL code cannot be sold. This is incorrect. If you release your code under a GPL license, however, you will have to provide access to your source code. You will also, if perusing GPLv3, need to provide a straightforward way to replace the GPLv3 elements in your product.

# What license should I pick for an internal product?

If you do not plan on distributing your product (for instance, a company's internal software) you do not, in fact, need to pick a license.

# My company is nervous about intellectual property

It is always a risk for open-source projects: what if the code contributed by one of the authors happens to be already licensed under an incompatible license? What if it was copied from a copyrighted product?

The authors of RootShell can assure you that its whole code base is original code and, basically, we are honest people. We do not, at this point, see a need for certificates of origin.

# Can a license be revoked?

Short answer: no. Should we decide, in a year from now, to revert our code to GPL only, this would only affect new code.

The code you are currently using will remain licensed under the exact same terms it is today.

# I have many more question

It is not our intent to write a complete FAQ that would take a complete week-end to read.

If you have questions, feel free to send them to Chris Ravenscroft: `chris-licenses [at] voilaweb.com`
