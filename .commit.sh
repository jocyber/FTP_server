echo "Adding to the staging area."
echo ""

git add ./*

read -p "Commit message: " message

echo ""
echo "Commiting to repository."
echo ""

git commit -m "$message"
git push origin main

echo ""
echo "End of commit."
