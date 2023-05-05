#!/bin/bash

set -x

TAG=$1
NEW_VERSION=${TAG#v}
echo "Preparing new library version $NEW_VERSION"

# Change directory to the root of the repository
cd "$(git rev-parse --show-toplevel)"


# Checkout to this branch
git config --global user.email "paclema@gmail.com"
git config --global user.name "Pablo Clemente"

git checkout "$TAG"
git checkout -b "release_v$NEW_VERSION"

git branch -r
git fetch

# Checkout to the branch where this tag is comming from:
# git checkout $(git describe --tags --abbrev=0 | cut -d'-' -f1)

# Update library.json version object
sed "/\"version\":/ { s/\(\"version\":\) \"\(.*\)\",/\1 \"${NEW_VERSION}\",/; :a; n; ba; }" library.json > tmp.json && mv tmp.json library.json


# Update CHANGELOG
DATE=$(date +%F)
UNDERLINE=$(printf -- '-%.0s' $(seq 1 ${#TAG}))
sed -i~ -bE "4s/HEAD/$TAG ($DATE)/; 5s/-+/$UNDERLINE/" CHANGELOG.md
rm CHANGELOG.md~

# Update PLatformIO badge in README
package_name=$(jq -r '.name' library.json)
package_owner=$(echo "${GITHUB_REPOSITORY}" | cut -d / -f 1)
BADGE_REGEX='\[!\[PlatformIO Registry\]\(https:\/\/badges\.registry\.platformio\.org\/packages\/'"${package_owner}"'\/library\/'"${package_name}"'\.svg\?version=[0-9]+\.[0-9]+\.[0-9]+\)\]'
BADGE_REPLACE='[![PlatformIO Registry](https://badges.registry.platformio.org/packages/'"${package_owner}"'/library/'"${package_name}"'.svg?version='"${NEW_VERSION}"')]'
sed -i -E "s#${BADGE_REGEX}#${BADGE_REPLACE}#g" README.md


# Commit file changes to git
git status
git add README.md CHANGELOG.md library.json
git commit -m "Update library to ${TAG}"
git status

git rebase master   # If the tag is not done under latest master commit, this rebase could fail

git checkout master
git merge release_v"$NEW_VERSION"

git push
# git push origin HEAD:$(git rev-parse --abbrev-ref HEAD)


# Update tag on last commit and add info
CHANGES=$(awk '/\* /{ FOUND=1; print; next } { if (FOUND) exit}' CHANGELOG.md)
# git tag "$TAG" "$TAG"^{} -f -m "$package_name $NEW_VERSION"$'\n'"$CHANGES"
# git push --follow-tags
git tag -d "$TAG"
git tag -a "$TAG" -m "$package_name $TAG"$'\n'"$CHANGES"
git push --force origin "$TAG"



set +x